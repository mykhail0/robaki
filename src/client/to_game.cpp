#include "to_game.h"

#include <arpa/inet.h>

#include <cstring>

client::to_game_t::to_game_t(session_id_t session_id, byte_t turn_direction,
                             uint32_t next_expected_event_no,
                             const std::string& player_name) {
  session_id = htobe64(session_id);
  size_t delta = sizeof session_id;
  byte_t* start = bytes.data();
  memcpy(start, &session_id, delta);
  start += delta;
  size_ += delta;

  delta = sizeof turn_direction;
  memcpy(start, &turn_direction, delta);
  start += delta;
  size_ += delta;

  next_expected_event_no = htonl(next_expected_event_no);
  delta = sizeof next_expected_event_no;
  memcpy(start, &next_expected_event_no, delta);
  start += delta;
  size_ += delta;

  delta = player_name.size();
  memcpy(start, player_name.c_str(), delta);
  size_ += delta;
}
