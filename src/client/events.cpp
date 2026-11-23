#include "events.h"

#include <cassert>
#include <cstring>

#include "../error.h"
#include "runtime_exceptions.h"

namespace {
// Return the number of digits of a given number.
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

void client::to_GUI_t::send_to_GUI(const socket_t& sockfd) {
  while (not finished()) {
    ssize_t sent = sockfd.send_data(data(), size());
    if (sent == -1) {
      if (errno == EWOULDBLOCK or errno == EAGAIN) {
        return;
      } else {
        throw critical_error(critical_error::kind::failed_send,
                             strerror(errno));
      }
    } else {
      off += sent;
    }
  }
}

client::new_game_t::new_game_t(const byte_t* to_parse, size_t len)
    : to_GUI_t(strlen("NEW_GAME ")) {
  size_t required = sizeof dimensions.x + sizeof dimensions.y;
  if (len < required or MAX_GAME_NEWGAME_MSG_LEN < len) {
    throw parsing_error(parsing_error::kind::invalid_length, "NEW_GAME");
  }

  size_t i = 0;

  dimensions.x = BEbytes_to_num<dim_t>(to_parse + i);
  i += sizeof dimensions.x;

  dimensions.y = BEbytes_to_num<dim_t>(to_parse + i);
  i += sizeof dimensions.y;

  std::string_view names_str((const char*)(to_parse + i), len - required);
  if (names_str.back() != '\0') {
    throw parsing_error(parsing_error::kind::invalid_format,
                        "No '\\0' at the end of player_name list.");
  }

  player_name_list.emplace_back();
  for (const auto c : names_str) {
    if (c == '\0') {
      player_name_list.emplace_back();
    } else {
      player_name_list.back().push_back(c);
    }
  }
  if (player_name_list.back().empty()) {
    player_name_list.pop_back();
  }
  player_name_list_is_ok();

  size_ += num_size<dim_t>(dimensions.x) + 1 /* space */ +
           num_size<dim_t>(dimensions.y) + 1 /* space */;

  for (const std::string& s : player_name_list) {
    size_ += s.size() + 1 /* space or new line */;
  }
}

void client::new_game_t::player_name_list_is_ok() const {
  if (player_name_list.size() < 2) {
    throw parsing_error(parsing_error::kind::invalid_format, "Too few players");
  }
  if (player_name_list.size() > MAX_PLAYERS_NUM) {
    throw parsing_error(parsing_error::kind::invalid_format,
                        "Too many players");
  }
  for (const std::string& s : player_name_list) {
    if (s.size() > MAX_PLAYER_NAME_LEN) {
      throw parsing_error(parsing_error::kind::invalid_format,
                          "Player name too long");
    }
    if (s.size() == 0) {
      throw parsing_error(parsing_error::kind::invalid_format,
                          "Empty player name");
    }
    for (const char c : s) {
      if (c < 33 or 126 < c) {
        throw parsing_error(parsing_error::kind::invalid_format,
                            "Bad player name");
      }
    }
  }
  for (size_t i = 1; i < player_name_list.size(); ++i) {
    if (player_name_list[i - 1] >= player_name_list[i]) {
      throw parsing_error(
          parsing_error::kind::invalid_format,
          "Player name list is not sorted in the increasing order");
    }
  }
}

// TODO rewrite without using intermediate std::string.
byte_t const* client::new_game_t::data() {
  if (not bytes.empty()) {
    return bytes.data() + off;
  }

  std::string str = "NEW_GAME ";
  str.append(std::to_string(dimensions.x))
      .append(" ")
      .append(std::to_string(dimensions.y))
      .append(" ");
  for (size_t j = 0; j + 1 < player_name_list.size(); ++j) {
    str.append(player_name_list[j]).append(" ");
  }
  str.append(player_name_list.back()).append("\n");

  assert(str.size() == size());
  bytes.resize(size());
  memcpy(bytes.data(), str.c_str(), str.size());
  return bytes.data();
}

client::pixel_t::pixel_t(const byte_t* to_parse, size_t len,
                         const std::vector<std::string>& names,
                         const dimensions_t& dimensions)
    : to_GUI_t(strlen("PIXEL ")) {
  if (len != GAME_PIXEL_MSG_LEN) {
    throw parsing_error(parsing_error::kind::invalid_length);
  }

  size_t i = 0;
  byte_t player_number = to_parse[i];
  i += sizeof player_number;

  if (player_number >= names.size()) {
    throw parsing_error(parsing_error::kind::invalid_player_number);
  }

  pos.x = BEbytes_to_num<dim_t>(to_parse + i);
  i += sizeof pos.x;

  pos.y = BEbytes_to_num<dim_t>(to_parse + i);
  if (not dimensions.contains(pos)) {
    throw parsing_error(parsing_error::kind::invalid_format,
                        "Out of board pixels");
  }

  player = names[player_number];
  size_ += num_size<dim_t>(pos.x) + 1 /* space */ + num_size<dim_t>(pos.y) +
           1 /* space */ + player.size() + 1 /* new line*/;
}

// TODO rewrite without using intermediate std::string.
byte_t const* client::pixel_t::data() {
  if (not bytes.empty()) {
    return bytes.data() + off;
  }

  std::string str = "PIXEL ";
  str.append(std::to_string(pos.x))
      .append(" ")
      .append(std::to_string(pos.y))
      .append(" ")
      .append(player)
      .append("\n");
  assert(size() == str.size());
  bytes.resize(size());
  memcpy(bytes.data(), str.c_str(), str.size());
  return bytes.data() + off;
}

client::player_eliminated_t::player_eliminated_t(
    const byte_t* to_parse, size_t len, const std::vector<std::string>& names)
    : to_GUI_t(strlen("PLAYER_ELIMINATED ")) {
  if (len != GAME_PLAYERELIMINATED_MSG_LEN) {
    throw parsing_error(parsing_error::kind::invalid_length);
  }
  byte_t player_number = to_parse[0];
  if (player_number >= names.size()) {
    throw parsing_error(parsing_error::kind::invalid_player_number);
  }

  player = names[player_number];
  size_ += player.size() + 1 /* new line */;
}

// TODO rewrite without using intermediate std::string.
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
