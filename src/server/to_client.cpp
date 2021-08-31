#include "to_client.h"
#include "../crc32.h"
#include <cstring>

uint32_t Event::cnt = 0;

NewGame::NewGame(dim_t maxx, dim_t maxy, size_t list_len,
    const char *player_name_list) : list_len(list_len) {
    byte_t *start = bytes;

    auto tmp = htonl(maxx);
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    tmp = htonl(maxy);
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    memcpy(start, player_name_list, list_len);
}

Pixel::Pixel(byte_t player_number, dim_t x, dim_t y) {
    byte_t *start = bytes;

    memcpy(start, &player_number, sizeof player_number);
    start += sizeof player_number;

    auto tmp = htonl(x);
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    tmp = htonl(y);
    memcpy(start, &tmp, sizeof tmp);
}

Event::Event(const EventData &event_data) {
    size_t data_len = event_data.size();
    auto event_type = event_data.event_type();
    uint32_t len = data_len + (sizeof Event::cnt) + sizeof event_type;
    byte_t *start = bytes;

    auto tmp = htonl(len);
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    tmp = htonl(Event::cnt++);
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    memcpy(start, &event_type, sizeof event_type);
    start += sizeof event_type;

    memcpy(start, event_data.data(), data_len);
    start += data_len;

    tmp = htonl(crc32(bytes, len + sizeof len));
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    sz = start - bytes;
}
