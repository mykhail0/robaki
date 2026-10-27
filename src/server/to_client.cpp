#include "to_client.h"

#include <cstring>

#include "../crc32.h"

uint32_t server::event_t::cnt = 0;

server::new_game_t::new_game_t(dim_t maxx, dim_t maxy, size_t list_len,
                               const char* player_name_list)
    : list_len(list_len) {
  byte_t* start = bytes.data();

  dim_t dimension = htonl(maxx);
  size_t delta = sizeof dimension;
  memcpy(start, &dimension, delta);
  start += delta;

  dimension = htonl(maxy);
  delta = sizeof dimension;
  memcpy(start, &dimension, delta);
  start += delta;

  memcpy(start, player_name_list, list_len);
}

server::pixel_t::pixel_t(byte_t player_number, dim_t x, dim_t y) {
  byte_t* start = bytes.data();

  size_t delta = sizeof player_number;
  memcpy(start, &player_number, delta);
  start += delta;

  dim_t pixel = htonl(x);
  delta = sizeof pixel;
  memcpy(start, &pixel, delta);
  start += delta;

  pixel = htonl(y);
  memcpy(start, &pixel, sizeof pixel);
}

server::event_t::event_t(const event_data_t& event_data) {
  byte_t event_type = event_data.event_type();
  uint32_t len =
      (sizeof event_t::cnt) + (sizeof event_type) + event_data.size();
  byte_t* start = bytes.data();

  uint32_t network_int = htonl(len);
  size_t delta = sizeof network_int;
  memcpy(start, &network_int, delta);
  start += delta;

  network_int = htonl(event_t::cnt++);
  delta = sizeof network_int;
  memcpy(start, &network_int, delta);
  start += delta;

  delta = sizeof event_type;
  memcpy(start, &event_type, delta);
  start += delta;

  delta = event_data.size();
  memcpy(start, event_data.data(), delta);
  start += delta;

  network_int = htonl(crc32(bytes.data(), (sizeof len) + len));
  delta = sizeof network_int;
  memcpy(start, &network_int, delta);
  start += delta;

  sz = start - bytes.data();
}
