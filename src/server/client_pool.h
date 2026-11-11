#ifndef CLIENT_POOL_H
#define CLIENT_POOL_H

#include <chrono>
#include <cstdint>
#include <map>
#include <unordered_set>

#include "../socket.h"
#include "game.h"

namespace server {
// Message queue between the server and a single client.
// TODO client should send next_expected after game
// over higher than that event no, so that server doesn't resend all of game
// history
class from_server_t {
 private:
  uint32_t ev_no;

 public:
  explicit from_server_t(uint32_t ev_no) : ev_no(ev_no) {}
  from_server_t() : from_server_t(0) {}

  // Lump together as many events as possible for a datagram
  // (from ev_no to as far as the `end`).
  // Return `true` iff sent something.
  bool send(const socket_t&, const sock_addr_t&, const state_t&);
  // Add messages for events starting from a given event number.
  void add(uint32_t new_ev_no) { ev_no = std::min(ev_no, new_ev_no); }
  bool empty(const state_t& game) const { return ev_no >= game.history.size(); }
};

struct client_t {
  session_id_t id;
  // When this deadline is older than chrono::now(),
  // this client_t should become invalid.
  std::chrono::time_point<std::chrono::system_clock> deadline;
  std::string player_name;
  // Last turn_direction.
  turn_direction_t turn_direction;
  // This player's worm index.
  ssize_t worm_ind = -1;
  // If a player is ready to start.
  bool ready = false;
  // Queue of messages to be sent to this client.
  from_server_t q;

  client_t(session_id_t, const std::string&, turn_direction_t turn_direction);
  // Call this whenever a datagram from this client is received to refresh
  // the deadline. Return if the player became ready.
  bool refresh(turn_direction_t turn_direction);
};

// Manage a collection of clients.
class client_pool_t {
 private:
  std::map<sock_addr_t, client_t> clients;
  std::unordered_set<std::string> names;

 public:
  using iterator = std::map<sock_addr_t, client_t>::iterator;
  using const_iterator = std::map<sock_addr_t, client_t>::const_iterator;
  size_t ready = 0;

  iterator find(const sock_addr_t& addr);
  const_iterator end() const { return clients.end(); }

  void add_client(sock_addr_t&& client_addr, client_t&& client);

  const_iterator remove_client(const_iterator);
  void remove_client(const sock_addr_t&);
  void remove_idle_clients();

  // For each client assign its alphabetical number and return the sorted list
  // of their names along with last turn_directions in that order. Reset clients
  // data before the game start.
  std::pair<std::string, std::vector<turn_direction_t>> game_ready();

  bool is_name_taken(const std::string&) const;
  size_t nonempty() const { return names.size(); }
  bool can_add_player() const;

  // Send a single message for some client with nonempty queue.
  void send(const socket_t& sockfd, const state_t& s);
  // For each client modify their message queues appropriately to broadcast
  // the round's events.
  void broadcast(uint32_t start);
  bool nonempty_queue(const state_t&) const;
};
}  // namespace server

#endif  // CLIENT_POOL_H
