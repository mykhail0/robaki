#include "from_game.h"

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

client::game_t::game_t(game_id_t gid, new_game_t&& e)
    : game_id(gid), smallest_not_sent(0), event_0(e) {
  add(0);
  // TODO could move the names out of it but it's an infrequent operation
  // anyways.
  names = e.get_names();
}

void client::game_t::add(uint32_t ev_no, std::unique_ptr<to_GUI_t> e) {
  add(ev_no);
  if (smallest_not_sent <= ev_no) {
    events_to_send[ev_no] = std::move(e);
  }
}

void client::game_t::send(const socket_t& sockfd) {
  if (empty()) {
    return;
  }

  if (smallest_not_sent == 0) {
    event_0.send_to_GUI(sockfd);
    if (not event_0.finished()) {
      return;
    }
    ++smallest_not_sent;
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

bool client::game_to_gui_t::empty() const {
  return not game.has_value() or game->empty();
}

void client::game_to_gui_t::send(const socket_t& gui_sockfd) {
  if (game) {
    game->send(gui_sockfd);
  }
}

void client::game_to_gui_t::receive(const socket_t& sockfd,
                                    const sock_addr_t& game_srvr) {
  ssize_t len;
  sockaddr_storage server_address;
  socklen_t rcv_len = sizeof server_address;
  if ((len = sockfd.receive(&server_address, &rcv_len, from_game)) <= 0) {
    return;
  }
  if (sock_addr_t(&server_address, rcv_len) != game_srvr) {
    std::cerr << "Received message from someone different than server."
              << std::endl;
    return;
  }
  if ((size_t)len > MAX_GAME_MSG_LEN) {
    throw parsing_error(parsing_error::kind::invalid_length,
                        "too long message from game server");
  }

  size_t offset = 0;
  game_id_t game_id = 0;
  read_num(game_id, from_game.data(), len, offset,
           "datagram smaller than game_id_t");

  // Three possible cases:
  // 1) Ignored from now on, but previous events are ok:
  // * bad checksum
  // * `len` field is smaller than `EVENT_LEN_WO_DATA`
  // 2) throw `parsing_error`
  // * good checksum, known event_type and nonsense data for the protocol
  //   (constructor for an event does that so we simply propagate)
  // * received datagram is truncated
  // (not enough bytes received to read the event message)
  // 3) Event processing is limited to just adding event_no:
  // * good checksum, but unknown event_type

  // TODO game over event offset == len, no?

  uint32_t event_no;
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
    // `offset` points at the start of the next event

    event_type_t event_type_ = event_type_t(event_type);
    // TODO Tricky. If missed NEW_GAME, then could be not accepting any new
    // stuff.
    // TODO store seen game IDs. If unseen game id then request 0 as next
    // event_no if not NEW_GAME

    switch (event_type_) {
      case event_type_t::new_game:
        // TODO here and other to_GUI set a flag for sending to GUI
        if (event_no != 0) {
          throw parsing_error(parsing_error::kind::invalid_format,
                              "NEW_GAME event with nonzero event number");
        }
        game.emplace(game_id, new_game_t(from_game.data() + event_data_start,
                                         event_data_len));
        break;
      case event_type_t::pixel: {
        std::unique_ptr<to_GUI_t> e = std::make_unique<pixel_t>(
            from_game.data() + event_data_start, event_data_len, game->names,
            game->get_dimensions());
        // TODO deduplicate this
        if (game.has_value() and game_id == game->id()) {
          game->add(event_no, std::move(e));
        }
      } break;
      case event_type_t::player_eliminated: {
        std::unique_ptr<to_GUI_t> e = std::make_unique<player_eliminated_t>(
            from_game.data() + event_data_start, event_data_len, game->names);
        if (game.has_value() and game_id == game->id()) {
          game->add(event_no, std::move(e));
        }
      } break;
      case event_type_t::game_over:
        // TODO prepare for NEW_GAME
        {
          game_over_t e(event_data_len);
        }
        if (game.has_value() and game_id == game->id()) {
          game->add(event_no);
        }
        break;
      default:
        // Unknown event type
        if (game.has_value() and game_id == game->id()) {
          game->add(event_no);
        }
        break;
    }
  }
}
