#ifndef SERVER_H
#define SERVER_H

#include <array>
#include <chrono>
#include <map>
#include <optional>
#include <unordered_map>
#include <unordered_set>

#include "../socket.h"
#include "client_pool.h"
#include "config.h"
#include "from_client.h"
#include "game.h"

namespace server {
struct server_t {
  socket_t sockfd;
  // + 1 is crucial for checking if the DGRAM sent by client is too large.
  std::array<byte_t, MAX_CLIENT_GAME_MSG_LEN + 1> buffer;

  const int rounds_per_sec;

  client_pool_t clients;
  state_t game;

  // Receive and parse a client's message. If `update_worm` then update the
  // client's worm's turn_direction.
  void receive(bool update_worm = false);
  // Decide what to do for a given message and client it is addressed from.
  // Return bool flags: ignore, add, remove
  std::tuple<bool, bool, bool> decide(client_pool_t::const_iterator it,
                                      const from_client_t& msg);

  void play_round();

  // Prepare the server for play_game().
  void game_ready();

  bool nonempty_queue() const { return clients.nonempty_queue(game); }

 public:
  server_t(socket_t&& sockfd, const config_t& config);
  // Exit when a game can be started.
  void wait_for_start();
  void play_game();
};
}  // namespace server

#endif /* SERVER_H */
