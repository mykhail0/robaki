#include "to_game.h"

#include <arpa/inet.h>

#include <cstring>

client::to_game_t::to_game_t(session_id_t session_id, byte_t turn_direction,
                             uint32_t next_expected_event_no,
                             const std::string& player_name)
    : sz(13) {
  session_id = htobe64(session_id);
  size_t l = sizeof session_id;
  byte_t* start = bytes.data();
  memcpy(start, &session_id, l);
  start += l;

  memcpy(start, &turn_direction, sizeof turn_direction);
  start += sizeof turn_direction;

  next_expected_event_no = htonl(next_expected_event_no);
  l = sizeof next_expected_event_no;
  memcpy(start, &next_expected_event_no, l);
  start += l;

  l = player_name.size();
  memcpy(start, player_name.c_str(), l);
  sz += l;
}
