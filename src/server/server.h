#ifndef SERVER_H
#define SERVER_H

#include <chrono>
#include <map>
#include <optional>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "../socket.h"
#include "config.h"
#include "game.h"

namespace server {
class server_t;

using clientkey_t = std::pair<SockAddr, session_id_t>;

// Message object to facilitate sending events information to clients.
// 2 usecases, depending on constructor, either send most recent events from the
// `start` to a single recipient, or broadcast events generated from a round.
class from_server_t {
  std::stack<clientkey_t> recipients;
  size_t ev_no;
  size_t start;
  std::optional<size_t> end;

 public:
  from_server_t(const clientkey_t& recipient, ssize_t start);
  from_server_t(std::stack<clientkey_t>&& recipients, ssize_t start,
                size_t end);

  // Lump together as many events as possible for a datagram
  // (from ev_no to as far as the `end`).
  // Then send it to the currently processed recipient.
  // When the recipient from the top of the stack receives everything,
  // it gets popped.
  void send(const server_t&);
  // Return `true` iff sent to everyone this message is meant for.
  bool done() const { return recipients.empty(); }
};

struct client_t {
  session_id_t id;
  // When this deadline is older than chrono::now(),
  // this client_t should become invalid.
  std::chrono::time_point<std::chrono::system_clock> deadline;
  // TODO is this rly needed?
  std::string player_name;

  client_t(session_id_t, const std::string&);
  // This function should be called
  // whenever a datagram from the client is received.
  void update_deadline();
  // TODO delete
  // bool operator<(const client_t &x) const { return player_name <
  // x.player_name;}
};

// Values for `names` map in wait_for_start() phase.
constexpr int NAMES_DEFAULT = -1, NAMES_READY = 1;

// Helper struct.
// A part of server_t::receive() return type.
// Holds useful information after the basic processing of a client's message.
struct received_t {
  byte_t turn_direction;
  std::string player_name;
  std::string removed;

  received_t() : turn_direction(0) {}
};

struct server_t {
  socket_t sockfd;
  std::array<byte_t, MAX_CLIENT_GAME_MSG_LEN + 1> buffer;

  const int rounds_per_sec;

  std::map<SockAddr, client_t> clients;
  // Different roles while waiting for a game to start and while running a game.
  // Never contains an empty string as a key.
  // Default uninitialized value is NAMES_DEFAULT.
  // 1) During readying for a game phase values are last turn_directions.
  // 2) During the game values are worm indexes.
  std::unordered_map<std::string, int> names;
  // Queue of message objects for sending messages to clients.
  // (advanced msg queue)
  std::queue<from_server_t> q;
  state_t game;

  // Return `true` iff the client is already associated with the server.
  bool contains(const clientkey_t&) const;
  // Add a client.
  void add(const SockAddr&, session_id_t, const std::string&);
  // Remove a client.
  void remove(const SockAddr&);
  // Update a client's deadline.
  void update(const SockAddr&);

  // Add a single type message to the queue for events from
  // the specified point up to the end of the game history.
  void add_msg(const clientkey_t&, uint32_t);
  // Add a broadcast type message to the queue for events from
  // the specified point up to the end of the game history.
  void add_broadcast_msg(uint32_t);

  // Helper function with mutual logic for wait_receive() and run_receive().
  std::pair<int, received_t> receive();

  // Receive and process a single message.
  // Modify the set of ready players accordingly,
  // which is passed as an argument.
  void wait_receive(std::unordered_set<std::string>&);
  void run_receive();

  // Send a single datagram, using `q`.
  void send();

  // Remove idle clients. If an additional `ready` set is given, remove from
  // there as well.
  void remove_idle_clients(std::unordered_set<std::string>* ready = nullptr);

  // Update the game state.
  void update_game();

  // Prepare the server for play_game().
  void game_ready();
  // Prepare the server for wait_for_start().
  void game_over();

 public:
  server_t(socket_t&& sockfd, const config_t& config);
  // Exit when a game can be started.
  void wait_for_start();
  void play_game();
};
}  // namespace server

#endif /* SERVER_H */
