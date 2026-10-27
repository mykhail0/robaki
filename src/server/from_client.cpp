#include "from_client.h"

#include <iostream>

#include "../utility.h"

std::string server::parsing_error::format_message(kind ec,
                                                  const std::string& detail) {
  std::string msg;
  switch (ec) {
    case kind::invalid_length:
      msg = "Bad datagram length";
      break;
    case kind::invalid_player_name:
      msg = "Invalid player name";
      break;
    case kind::bad_turn_direction:
      msg = "Invalid turn direction";
      break;
    default:
      msg = "Unknown parsing error";
      break;
  }
  if (not detail.empty()) {
    msg.append(": ").append(detail);
  }
  return msg;
}

server::parsing_error::parsing_error(kind ec, const std::string& detail)
    : std::runtime_error(format_message(ec, detail)), error_kind_(ec) {}

server::from_client_t::from_client_t(byte_t* bytes, size_t len) {
  size_t required = (sizeof session_id) + (sizeof turn_direction) +
                    (sizeof next_expected_event_no);
  if (len < required or MAX_CLIENT_GAME_MSG_LEN < len) {
    throw parsing_error(parsing_error::kind::invalid_length);
  }

  size_t i = 0;

  session_id = BEbytes_to_num<session_id_t>(bytes + i);
  i += sizeof session_id;

  turn_direction = bytes[i];
  i += sizeof turn_direction;
  if (turn_direction != STRAIGHT and
      (turn_direction != LEFT and turn_direction != RIGHT)) {
    throw parsing_error(parsing_error::kind::bad_turn_direction);
  }

  next_expected_event_no = BEbytes_to_num<uint32_t>(bytes + i);
  i += sizeof next_expected_event_no;

  player_name = std::string((const char*)(bytes + i), len - required);
  for (auto c : player_name) {
    if (c < 33 or 126 < c) {
      throw parsing_error(parsing_error::kind::invalid_player_name);
    }
  }

  if (required + player_name.size() < len) {
    throw parsing_error(parsing_error::kind::invalid_length);
  }
}

void server::from_client_t::print() const {
  std::cerr << "Received message from client: "
            << "session id: " << session_id
            << " turn direction: " << turn_direction
            << " next expected event no: " << next_expected_event_no
            << " player name: " << player_name << "\n";
}
