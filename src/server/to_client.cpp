#include "to_client.h"

#include <cstring>

#include "../crc32.h"

uint32_t server::event_t::cnt = 0;

server::event_t::event_t(dim_t maxx, dim_t maxy, size_t list_len,
                         const char* player_name_list)
    : event_t(new_game_t(list_len)) {
  byte_t* dst = bytes.data() + FIELD_LEN_BEFORE_EVENT_DATA;

  dim_t dimension = htonl(maxx);
  size_t delta = sizeof dimension;
  memcpy(dst, &dimension, delta);
  dst += delta;

  dimension = htonl(maxy);
  delta = sizeof dimension;
  memcpy(dst, &dimension, delta);
  dst += delta;

  memcpy(dst, player_name_list, list_len);
  add_crc();
}

server::event_t::event_t(byte_t player_number, dim_t x, dim_t y)
    : event_t(pixel_t()) {
  byte_t* dst = bytes.data() + FIELD_LEN_BEFORE_EVENT_DATA;

  size_t delta = sizeof player_number;
  memcpy(dst, &player_number, delta);
  dst += delta;

  dim_t pixel = htonl(x);
  delta = sizeof pixel;
  memcpy(dst, &pixel, delta);
  dst += delta;

  pixel = htonl(y);
  memcpy(dst, &pixel, sizeof pixel);
  add_crc();
}

server::event_t::event_t(byte_t player) : event_t(player_eliminated_t()) {
  bytes[FIELD_LEN_BEFORE_EVENT_DATA] = player;
  add_crc();
}

server::event_t::event_t() : event_t(game_over_t()) { add_crc(); }

server::event_t::event_t(const event_data_t& event_data) {
  byte_t event_type = event_data.event_type();
  len = (sizeof event_t::cnt) + (sizeof event_type) + event_data.size();
  byte_t* dst = bytes.data();

  uint32_t network_int = htonl(len);
  size_t delta = sizeof network_int;
  memcpy(dst, &network_int, delta);
  dst += delta;

  network_int = htonl(event_t::cnt++);
  delta = sizeof network_int;
  memcpy(dst, &network_int, delta);
  dst += delta;

  delta = sizeof event_type;
  memcpy(dst, &event_type, delta);
  dst += delta;
}

void server::event_t::add_crc() {
  uint32_t network_int = htonl(crc32(bytes.data(), (sizeof len) + len));
  memcpy(bytes.data() + len, &network_int, sizeof network_int);
}
