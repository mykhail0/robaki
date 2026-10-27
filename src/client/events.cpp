#include "events.h"

#include <cstring>
#include <queue>

#include "../error.h"
#include "runtime_exceptions.h"

namespace {
// Returns the number of digits of a given number.
template <class T>
size_t num_size(T x) {
  if (x == 0) {
    return 1;
  }
  size_t i = 0;
  while (x != 0) {
    x /= 10;
    ++i;
  }
  return i;
}
}  // namespace

bool client::to_GUI_t::send_to_GUI(const socket_t& sockfd) {
  ssize_t sent = 0;
  while (not finished()) {
    sent = send(sockfd.get(), data(), size(), 0);
    int err = errno;
    if (sent != -1) {
      offset(sent);
    } else {
      if (err == EWOULDBLOCK or err == EAGAIN) {
        return false;
      } else {
        throw critical_error(critical_error::kind::failed_send, strerror(err));
      }
    }
  }
  return true;
}

// "NEW_GAME___".size() == 11
client::new_game_t::new_game_t(byte_t* to_parse, size_t len) : to_GUI_t(11) {
  size_t required = sizeof maxx + sizeof maxy;
  if (len < required or MAX_GAME_NEWGAME_MSG_LEN < len) {
    throw parsing_error(parsing_error::kind::invalid_length, "NEW_GAME");
  }

  size_t i = 0;

  maxx = BEbytes_to_num<dim_t>(to_parse + i);
  i += sizeof maxx;

  maxy = BEbytes_to_num<dim_t>(to_parse + i);
  i += sizeof maxy;

  std::string names_str((const char*)(to_parse + i), len - required);
  if (names_str.back() != 0) {
    throw parsing_error(parsing_error::kind::invalid_format,
                        "No '\\0' at the end of player_name list.");
  }
  std::queue<int> null_ind;
  for (auto c : names_str) {
    if (c == 0) {
      null_ind.push(i);
    }
    ++i;
  }

  for (i = 0; not null_ind.empty(); null_ind.pop()) {
    player_name_list.push_back(std::string(names_str, null_ind.front() - i));
    i = null_ind.front() + 1;
    if (player_name_list.back().size() > MAX_PLAYER_NAME_LEN) {
      throw parsing_error(parsing_error::kind::invalid_format,
                          "Too long player_name");
    }
  }

  if (player_name_list.size() > MAX_PLAYERS_NUM) {
    throw parsing_error(parsing_error::kind::invalid_format,
                        "Too many players");
  }

  sz += num_size<dim_t>(maxx) + num_size<dim_t>(maxy);

  for (auto& s : player_name_list) {
    sz += s.size() + 1;
    for (auto c : s) {
      if (c < 33 or 126 < c) {
        throw parsing_error(parsing_error::kind::invalid_format,
                            "Bad player name");
      }
    }
  }
}

byte_t const* client::new_game_t::data() {
  if (not bytes.empty()) {
    return bytes.data() + off;
  }

  std::string str = "NEW_GAME";
  str.append(std::to_string(maxx))
      .append(" ")
      .append(std::to_string(maxy))
      .append(" ");
  for (size_t j = 0; j + 1 < player_name_list.size(); ++j) {
    str.append(player_name_list[j]).append(" ");
  }
  str.append(player_name_list.back()).append("\n");

  bytes.resize(size());
  memcpy(bytes.data(), str.c_str(), str.size());
  return bytes.data() + off;
}

// "PIXEL____".size() == 9
client::pixel_t::pixel_t(byte_t* to_parse, size_t len,
                         const std::unordered_map<byte_t, std::string>& names)
    : to_GUI_t(9) {
  if (len != GAME_PIXEL_MSG_LEN) {
    throw parsing_error(parsing_error::kind::invalid_length);
  }
  size_t i = 0;

  auto player_number = to_parse[i];
  i += sizeof player_number;

  if (player_number > MAX_PLAYERS_NUM) {
    throw parsing_error(parsing_error::kind::invalid_player_number);
  }

  x = BEbytes_to_num<dim_t>(to_parse + i);
  i += sizeof x;

  y = BEbytes_to_num<dim_t>(to_parse + i);

  player = names.at(player_number);
  sz += num_size<dim_t>(x) + num_size<dim_t>(y) + player.size();
}

byte_t const* client::pixel_t::data() {
  if (not bytes.empty()) {
    return bytes.data() + off;
  }

  std::string str = "PIXEL ";
  str.append(std::to_string(x))
      .append(" ")
      .append(std::to_string(y))
      .append(" ")
      .append(player)
      .append("\n");
  bytes.resize(size());
  memcpy(bytes.data(), str.c_str(), str.size());
  return bytes.data() + off;
}

client::player_eliminated_t::player_eliminated_t(
    byte_t* to_parse, size_t len,
    const std::unordered_map<byte_t, std::string>& names)
    : to_GUI_t(19) {
  if (len != GAME_PLAYERELIMINATED_MSG_LEN) {
    throw parsing_error(parsing_error::kind::invalid_length);
  }
  auto player_number = to_parse[0];
  if (player_number > MAX_PLAYERS_NUM) {
    throw parsing_error(parsing_error::kind::invalid_player_number);
  }

  player = names.at(player_number);
  sz += player.size();
}

byte_t const* client::player_eliminated_t::data() {
  if (not bytes.empty()) {
    return bytes.data() + off;
  }

  std::string str = "PLAYER_ELIMINATED ";
  str.append(player).append("\n");
  bytes.resize(size());
  memcpy(bytes.data(), str.c_str(), str.size());
  return bytes.data() + off;
}

client::game_over_t::game_over_t(size_t len) {
  if (len != GAME_GAMEOVER_MSG_LEN) {
    throw parsing_error(parsing_error::kind::invalid_length);
  }
}
