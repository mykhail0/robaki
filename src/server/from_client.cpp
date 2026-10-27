#include "from_client.h"

#include <iostream>

#include "../utility.h"

server::from_client_t::from_client_t(byte_t* bytes, size_t len) {
  size_t required = (sizeof session_id) + (sizeof turn_direction) +
                    (sizeof next_expected_event_no);
  if (len < required or MAX_CLIENT_GAME_MSG_LEN < len) {
    throw std::invalid_argument("bad client datagram length");
  }

  size_t i = 0;

  session_id = BEbytes_to_num<session_id_t>(bytes + i);
  i += sizeof session_id;

  turn_direction = bytes[i];
  i += sizeof turn_direction;
  if (turn_direction != STRAIGHT and
      (turn_direction != LEFT and turn_direction != RIGHT)) {
    throw std::invalid_argument("bad turn direction");
  }

  next_expected_event_no = BEbytes_to_num<uint32_t>(bytes + i);
  i += sizeof next_expected_event_no;

  player_name = std::string((const char*)(bytes + i), len - required);
  for (auto c : player_name) {
    if (c < 33 or 126 < c) {
      throw std::invalid_argument("bad player name");
    }
  }

  if (required + player_name.size() < len) {
    throw std::invalid_argument("bad client datagram length");
  }
}

void server::from_client_t::print() const {
  std::cerr << "Received message from client: "
            << "session id: " << session_id
            << " turn direction: " << turn_direction
            << " next expected event no: " << next_expected_event_no
            << " player name: " << player_name << "\n";
}
