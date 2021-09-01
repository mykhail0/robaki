#ifndef CONSTS_H
#define CONSTS_H

#include <cstddef>
#include <cstdint>

// Type definitions for fields used in protocol datagrams.

using byte_t = uint8_t;
// Dimension type. Although practically does not exceed uint16_t.
using dim_t = uint32_t;
using game_id_t = uint32_t;
// time_t ?
using session_id_t = uint64_t;

/* Maximal event_data length of NEW_GAME message from server to client is:
25 (maximal players number) * 21 (maximal player_name's length +
1 byte for '\0') + 4 (maxx) + 4 (maxy) = 533 */
constexpr size_t MAX_GAME_NEWGAME_MSG_LEN = 533,
// Similarly.
    GAME_PIXEL_MSG_LEN = 9,
    GAME_PLAYERELIMINATED_MSG_LEN = 1,
    GAME_GAMEOVER_MSG_LEN = 0;

// MAX_SRVR_NEWGAME_MSG_LEN + 17 (len, event_no, type, crc32).
constexpr size_t MAX_GAME_EVENT_MSG_LEN = 546;

/* MAX_SRVR_EVENT_MSG_LEN + 4 (game_id) */
constexpr size_t MAX_GAME_MSG_LEN = 550;

constexpr size_t MAX_PLAYERS_NUM = 25;

// Maximal message a client can send to the game server.
constexpr size_t MAX_2_GAME_MSG_LEN = 33;

// turn_direction
constexpr byte_t STRAIGHT = 0,
    RIGHT = 1,
    LEFT = 2;

// event_type
constexpr byte_t NEW_GAME = 0,
    PIXEL = 1,
    PLAYER_ELIMINATED = 2,
    GAME_OVER = 3;

#endif /* CONSTS_H */
