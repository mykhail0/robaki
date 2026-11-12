#include "from_game.h"

#include <cassert>
#include <iostream>

#include "../crc32.h"
#include "runtime_exceptions.h"

namespace {
// A wrapper for T BEbytes_to_num() in the context of
// reading data from a specific buffer. `len` is the size of the buffer,
// `offset` is it's current offset.
// `offset` is advanced accordingly.
// Throw `parsing_error` if data is not long enough to read the value.
template <class T>
void read_num(T& ret, byte_t* bytes, size_t len, size_t& offset,
              const std::string& err_msg = "") {
  size_t type_sz = sizeof(T);
  if (len < offset + type_sz) {
    throw client::parsing_error(client::parsing_error::kind::invalid_length,
                                err_msg);
  }
  ret = BEbytes_to_num<T>(bytes + offset);
  offset += type_sz;
}

// Just a call to `read_num()` with a specific error message.
template <class T>
void read_num_from_event(T& ret, byte_t* bytes, size_t len, size_t& offset) {
  return read_num(ret, bytes, len, offset,
                  "Not enough bytes received to read the event message.");
}

constexpr size_t EVENT_LEN_WO_DATA =
    sizeof(uint32_t) /* event_no */ + sizeof(byte_t) /* event_type */;
}  // namespace

void client::game_t::reset(game_id_t gid) {
  if (game_id.has_value()) {
    old_game_ids.insert(game_id.value());
  }
  game_id.emplace(gid);
  smallest_not_sent = 0;
  event_nums.clear();
  dimensions.reset();
  names.clear();
}

uint32_t client::game_t::next_expected_event_no() const {
  return event_nums.smallest_not_included();
}

void client::game_t::add(uint32_t ev_no, std::unique_ptr<to_GUI_t> e) {
  if (smallest_not_sent <= ev_no) {
    add(ev_no);
    events_to_send.emplace(ev_no, std::move(e));
  }
}

void client::game_t::add(std::unique_ptr<new_game_t> e) {
  if (names.empty() or not dimensions.has_value()) {
    assert(names.empty() and not dimensions.has_value());
    dimensions.emplace(e->get_dimensions());
    names = e->player_name_list;
  }
  add(0, std::move(e));
}

void client::game_t::send(const socket_t& sockfd) {
  if (empty()) {
    return;
  }
  uint32_t next_ev_no = next_expected_event_no();
  for (std::map<uint32_t, std::unique_ptr<to_GUI_t>>::iterator it =
           events_to_send.begin();
       it != events_to_send.end() and it->first < next_ev_no;
       ++smallest_not_sent) {
    it->second->send_to_GUI(sockfd);
    if (not it->second->finished()) {
      return;
    }
    it = events_to_send.erase(it);
  }
}

bool client::game_t::empty() const {
  return next_expected_event_no() <= smallest_not_sent;
}

uint32_t client::game_to_gui_t::next_expected_event_no() const {
  return game.next_expected_event_no();
}

ssize_t client::game_to_gui_t::receive_packet(
    const socket_t& sockfd, const sock_addr_t& game_server_addr) {
  sockaddr_storage server_address;
  socklen_t rcv_len = sizeof server_address;
  ssize_t len = sockfd.receive(&server_address, &rcv_len, from_game);
  if (len <= 0) {
    return len;
  }
  if (sock_addr_t(&server_address, rcv_len) != game_server_addr) {
    std::cerr << "Received message from someone different than server."
              << std::endl;
    return -1;
  }
  if ((size_t)len > MAX_GAME_MSG_LEN) {
    throw parsing_error(parsing_error::kind::invalid_length,
                        "too long message from game server");
  }
  return len;
}

void client::game_to_gui_t::handle_event(event_type_t event_type,
                                         game_id_t game_id, uint32_t event_no,
                                         const byte_t* data, size_t data_len) {
  bool old_game_id = game.old_game_ids.count(game_id) == 1;
  if (not old_game_id and
      (not game.game_id.has_value() or game.game_id.value() != game_id)) {
    game.reset(game_id);
  }
  if (event_type == event_type_t::new_game) {
    if (event_no != 0) {
      throw parsing_error(parsing_error::kind::invalid_format,
                          "NEW_GAME event with nonzero event number");
    }
    std::unique_ptr<new_game_t> e =
        std::make_unique<new_game_t>(data, data_len);
    if (not old_game_id) {
      game.add(std::move(e));
    }
  } else if (event_type == event_type_t::pixel or
             event_type == event_type_t::player_eliminated) {
    if (not game.names.empty() or game.dimensions.has_value()) {
      assert(not game.names.empty() and game.dimensions.has_value());
      std::unique_ptr<to_GUI_t> e;
      if (event_type == event_type_t::pixel) {
        e = std::make_unique<pixel_t>(data, data_len, game.names,
                                      game.dimensions.value());
      } else {
        e = std::make_unique<player_eliminated_t>(data, data_len, game.names);
      }
      if (not old_game_id) {
        game.add(event_no, std::move(e));
      }
    }
  } else {
    // GAME OVER or unknown event type
    if (event_type == event_type_t::game_over) {
      game_over_t e(data_len);
    }
    if (not old_game_id) {
      game.add(event_no);
    }
  }
}

// Possible actions for a packet:
// 0) Ignore the packet:
//    * comes from an unknown source
// 1) Events ignored from now on, but previous events are ok:
//    * bad checksum
// 2) Throw `parsing_error`
//    * good checksum, known `event_type` and nonsense data for the protocol
//      (constructor for an event does that so we simply propagate)
//    * received datagram is truncated
//      (not enough bytes received to read the event message)
//    * `len` value is smaller than `EVENT_LEN_WO_DATA`
// 3) Event processing is limited to just adding `event_no`:
//    * good checksum, but unknown `event_type`
void client::game_to_gui_t::receive(const socket_t& sockfd,
                                    const sock_addr_t& game_server_addr) {
  ssize_t len = receive_packet(sockfd, game_server_addr);
  if (len <= 0) {
    return;
  }
  size_t offset = 0;
  game_id_t game_id = 0;
  read_num(game_id, from_game.data(), len, offset,
           "datagram smaller than game_id_t");

  uint32_t event_no;
  // `offset` points at the start of the next event
  for (std::optional<uint32_t> previous_event_no; offset < (size_t)len;
       previous_event_no = event_no) {
    size_t event_start = offset;
    uint32_t event_len, checksum;
    byte_t event_type;

    read_num_from_event(event_len, from_game.data(), len, offset);
    if (event_len < EVENT_LEN_WO_DATA) {
      throw parsing_error(parsing_error::kind::invalid_length,
                          "event too short");
    }
    read_num_from_event(event_no, from_game.data(), len, offset);
    if (previous_event_no.has_value()) {
      if (previous_event_no.value() >= event_no or
          previous_event_no.value() + 1 != event_no) {
        throw parsing_error(parsing_error::kind::invalid_format,
                            "game events out of order inside a packet");
      }
    }
    read_num_from_event(event_type, from_game.data(), len, offset);

    size_t event_data_start = offset,
           event_data_len = event_len - EVENT_LEN_WO_DATA;
    offset += event_data_len;
    read_num_from_event(checksum, from_game.data(), len, offset);
    if (crc32(from_game.data() + event_start, sizeof event_len + event_len) !=
        checksum) {
      return;
    }
    handle_event(event_type_t(event_type), game_id, event_no,
                 from_game.data() + event_data_start, event_data_len);
  }
}
