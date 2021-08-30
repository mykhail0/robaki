#ifndef CONSTS_H
#define CONSTS_H

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
constexpr int MAX_GAME_NEWGAME_MSG_LEN = 533;
// Similarly.
constexpr int GAME_PIXEL_MSG_LEN = 9;
constexpr int GAME_PLAYERELIMINATED_MSG_LEN = 1;
constexpr int GAME_GAMEOVER_MSG_LEN = 0;

// MAX_SRVR_NEWGAME_MSG_LEN + 17 (len event_no type crc32).
constexpr int MAX_GAME_EVENT_MSG_LEN = 546;

/* MAX_SRVR_EVENT_MSG_LEN + 4 (game_id) */
constexpr int MAX_GAME_MSG_LEN = 550;

constexpr int MAX_GAMERS_NUM = 25;

// Maximal message a client can send to the game server.
constexpr int MAX_2_GAME_MSG_LEN = 33;

// turn_direction
constexpr byte_t STRAIGHT = 0;
constexpr byte_t RIGHT = 1;
constexpr byte_t LEFT = 2;

// event_type
constexpr byte_t NEW_GAME = 0;
constexpr byte_t PIXEL = 1;
constexpr byte_t PLAYER_ELIMINATED = 2;
constexpr byte_t GAME_OVER = 3;

#endif /* CONSTS_H */
