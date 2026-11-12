#include "to_client.h"

#include <cstring>

#include "../crc32.h"

uint32_t server::event_t::cnt = 0;

server::event_t::event_t(const event_data_t& event_data) {
  byte_t event_type = event_data.event_type();
  len = (sizeof event_t::cnt) + (sizeof event_type) + event_data.size();
  bytes.resize(size());
  byte_t* dst = bytes.data();

  uint32_t network_int = htonl(len);
  size_t delta = sizeof network_int;
  memcpy(dst, &network_int, delta);
  dst += delta;

  network_int = htonl(event_t::cnt++);
  delta = sizeof network_int;
  memcpy(dst, &network_int, delta);
  dst += delta;

  memcpy(dst, &event_type, sizeof event_type);
}

void server::event_t::add_crc() {
  size_t crc_start = len + sizeof len;
  uint32_t network_int = htonl(crc32(bytes.data(), crc_start));
  memcpy(bytes.data() + crc_start, &network_int, sizeof network_int);
}

server::event_t::event_t(dimensions_t dimensions,
                         const std::string& player_name_list)
    : event_t(new_game_t(player_name_list.size())) {
  byte_t* dst = bytes.data() + FIELD_LEN_BEFORE_EVENT_DATA;

  dim_t dimension = htonl(dimensions.x);
  size_t delta = sizeof dimension;
  memcpy(dst, &dimension, delta);
  dst += delta;

  dimension = htonl(dimensions.y);
  delta = sizeof dimension;
  memcpy(dst, &dimension, delta);
  dst += delta;

  memcpy(dst, player_name_list.c_str(), player_name_list.size());
  add_crc();
}

server::event_t::event_t(byte_t player_number, dimensions_t pos)
    : event_t(pixel_t()) {
  byte_t* dst = bytes.data() + FIELD_LEN_BEFORE_EVENT_DATA;

  size_t delta = sizeof player_number;
  memcpy(dst, &player_number, delta);
  dst += delta;

  dim_t pixel = htonl(pos.x);
  delta = sizeof pixel;
  memcpy(dst, &pixel, delta);
  dst += delta;

  pixel = htonl(pos.y);
  memcpy(dst, &pixel, sizeof pixel);
  add_crc();
}

server::event_t::event_t(byte_t player) : event_t(player_eliminated_t()) {
  bytes[FIELD_LEN_BEFORE_EVENT_DATA] = player;
  add_crc();
}

server::event_t::event_t() : event_t(game_over_t()) { add_crc(); }
