#ifndef CONSTS_H
#define CONSTS_H

#include <algorithm>
#include <cstddef>
#include <cstdint>

// Type definitions for fields used in protocol datagrams.

using byte_t = uint8_t;
// Dimension type. Although in practice does not exceed `uint16_t`.
using dim_t = uint32_t;
using game_id_t = uint32_t;
// TODO time_t ?
using session_id_t = uint64_t;

constexpr size_t MAX_PLAYERS_NUM = 25, MAX_PLAYER_NAME_LEN = 20;

// Maximum `event_data` length of `NEW_GAME` message from server to client.
constexpr size_t MAX_GAME_NEWGAME_MSG_LEN =
                     MAX_PLAYERS_NUM * (MAX_PLAYER_NAME_LEN + 1 /* '\0' */) +
                     sizeof(dim_t) /* maxx */ + sizeof(dim_t) /* maxy */,
                 // `PIXEL` message length
    GAME_PIXEL_MSG_LEN = sizeof(byte_t) /* player_number */ +
                         sizeof(dim_t) /* x */ + sizeof(dim_t) /* y */,
                 // `PLAYER_ELIMINATED` message length
    GAME_PLAYERELIMINATED_MSG_LEN = sizeof(byte_t) /* player_number */,
                 // `GAME_OVER` message length
    GAME_GAMEOVER_MSG_LEN = 0;

constexpr size_t MAX_GAME_EVENT_MSG_LEN =
    std::max({MAX_GAME_NEWGAME_MSG_LEN, GAME_PIXEL_MSG_LEN,
              GAME_PLAYERELIMINATED_MSG_LEN, GAME_GAMEOVER_MSG_LEN}) +
    sizeof(uint32_t) /* len */ + sizeof(uint32_t) /* event_no */ +
    sizeof(byte_t) /* event_type */ + sizeof(uint32_t) /* crc32 */;

constexpr size_t MAX_GAME_MSG_LEN =
    MAX_GAME_EVENT_MSG_LEN + sizeof(game_id_t) /* game_id */;

constexpr size_t MAX_CLIENT_GAME_MSG_LEN =
    sizeof(session_id_t) /* session_id */ +
    sizeof(byte_t) /* turn_direction */ +
    sizeof(uint32_t) /* next_expected_event_no */ + MAX_PLAYER_NAME_LEN;

// TODO enum classes for these?
// turn_direction
constexpr byte_t STRAIGHT = 0, RIGHT = 1, LEFT = 2;

// event_type
constexpr byte_t NEW_GAME = 0, PIXEL = 1, PLAYER_ELIMINATED = 2, GAME_OVER = 3;

#endif /* CONSTS_H */
