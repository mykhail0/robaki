#ifndef FROM_GAME_H
#define FROM_GAME_H

#include <array>
#include <cstdint>
// TODO
#include <iostream>
#include <map>
#include <memory>
#include <optional>

#include "../socket.h"
#include "events.h"
#include "iset.h"

namespace client {
// A class responsible for managing receiving messages from the game server and
// sending them to the GUI server in the context of a single game.
// So called event buffer.
class game_t {
  game_id_t game_id;

  // Interval set is used for efficiently extracting the next_expected_event_no.
  iset event_nums;
  // The smallest event number which still wasn't sent to the GUI server.
  uint32_t smallest_not_sent;
  // Events still not sent to the GUI server. Keys are event numbers.
  std::map<uint32_t, std::unique_ptr<to_GUI_t>> events_to_send;

  // The NEW_GAME event.
  new_game_t event_0;

 public:
  game_t(game_id_t, new_game_t&&);

  game_id_t id() const { return game_id; }
  // Add an event to the event buffer.
  void add(uint32_t ev_no) { event_nums.insert(ev_no); }
  void add(uint32_t, std::unique_ptr<to_GUI_t>);

  // Smallest event number which wasn't received from the game server.
  uint32_t next_expected_event_no() const {
    return event_nums.smallest_not_included();
  }

  // Send events not sent, up to the first hole in history.
  void send(const socket_t&);
  // There's nothing to send to GUI.
  bool empty() const;
  // Enumeration of players for this game.
  std::vector<std::string> names;
  std::pair<dim_t, dim_t> get_dimensions() const {
    return event_0.get_dimensions();
  }
};

class game_to_gui_t {
 private:
  std::optional<game_t> game;
  // Buffer for receiving messages from the game server.
  // + 1 is crucial for checking if datagram from the server is too large.
  std::array<byte_t, MAX_GAME_MSG_LEN + 1> from_game;

 public:
  // Receive a message from a given socket fd (of the game server)
  // and integrate it into the <Game> queue.
  void receive(const socket_t&, const sock_addr_t&);
  // Send data to the GUI server.
  void send(const socket_t& gui_sockfd);
  // There's nothing to send to GUI.
  bool empty() const;

  // next_expected_event_no
  uint32_t expected() const {
    return game ? game->next_expected_event_no() : 0;
  }

  //
};
}  // namespace client

#endif /* FROM_GAME_H */
