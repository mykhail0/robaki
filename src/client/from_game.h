#ifndef FROM_GAME_H
#define FROM_GAME_H

#include <cstdint>
#include <map>
#include <memory>
#include <queue>
#include <unordered_set>

#include "../socket.h"
#include "events.h"
#include "iset.h"

namespace client {
// A class responsible for managing receiving messages from the game server and
// sending them to the GUI server in the context of a single game.
// So called event buffer.
class game_t {
  game_id_t game_id;

  // Interval set is used for
  // efficiently extracting the next_expected_event_no.
  iset event_nums;
  // The smallest event number which still wasn't sent to the GUI server.
  uint32_t smallest_not_sent;
  // Events still not sent to the GUI server. Keys are event numbers.
  std::map<uint32_t, std::unique_ptr<to_GUI_t>> received;

  // The NEW_GAME event.
  new_game_t ev0;

  // See bool Game::is_sending_the_rest() const;
  bool sending_the_rest;

 public:
  game_t(game_id_t, const new_game_t&);

  game_id_t id() const { return game_id; }
  // Adds event to the event buffer.
  void add(uint32_t, std::unique_ptr<to_GUI_t>);

  // next_expected_event_no for this game.
  // Smallest event number which wasn't received from the game server.
  uint32_t expected() const { return event_nums.smallest_not_included(); }

  // 2 functions below return `true` iff everything was sent successfully.
  // Return `false` if there is still something to be sent but the socket would
  // block. Otherwise throws `critical_exception`.
  // Receive a socket to send data to.

  // Sends events not sent, up to the first hole in history.
  bool send(const socket_t&);
  // Sends all events not sent.
  bool send_the_rest(const socket_t&);

  // Returns true iff send_the_rest() was already called.
  bool is_sending_the_rest() const { return sending_the_rest; }
  // Numeration of players for this game.
  std::unordered_map<byte_t, std::string> names;
};

class game_to_gui_t {
 private:
  // Queue of game buffers.
  // Received messages can influence only the q.back().
  // Other elements are there to be sent to the GUI server.
  std::queue<game_t> q;
  // See ctor.
  std::unordered_set<byte_t> known_event_types;
  // Buffer for receiving messages from the game server.
  byte_t from_game[MAX_GAME_MSG_LEN + 1];

 public:
  game_to_gui_t()
      : known_event_types({NEW_GAME, PIXEL, PLAYER_ELIMINATED, GAME_OVER}) {}

  bool event_is_known(byte_t) const;

  // Receives a message from a given socket fd (of the game server)
  // and integrates it into the <Game> queue.
  void recv(const socket_t&, const SockAddr&);
  // Sends data to the GUI server.
  // Returns `false` if there is still something to be sent
  // but the socket would block. Otherwise exits(1).
  // Receives a socket to send data to.
  // TODO check for random exits in the codebase like this
  bool send(const socket_t&);

  // next_expected_event_no
  uint32_t expected() const { return q.empty() ? 0 : q.back().expected(); }
};
}  // namespace client

#endif /* FROM_GAME_H */
