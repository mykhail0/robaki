#ifndef FROM_GAME_H
#define FROM_GAME_H

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <unordered_set>

#include "../socket.h"
#include "events.h"
#include "iset.h"

namespace client {
// A class responsible for managing receiving messages from the game server and
// sending them to the GUI server in the context of a single game.
// So called event buffer.
class game_t {
  // Interval set is used for efficiently extracting the next_expected_event_no.
  iset event_nums;
  // The smallest event number which still wasn't sent to the GUI server.
  uint32_t smallest_not_sent = 0;
  // Events still not sent to the GUI server. Keys are event numbers.
  std::map<uint32_t, std::unique_ptr<to_GUI_t>> events_to_send;

 public:
  std::unordered_set<game_id_t> old_game_ids;
  std::optional<game_id_t> game_id;
  // `dimensions` and `names` are always coupled and set together with
  // NEW GAME events.
  std::optional<dimensions_t> dimensions;
  // Enumeration of players for this game.
  std::vector<std::string> names;

  game_t() = default;
  void reset(game_id_t);

  // Add an event to the event buffer.
  void add(uint32_t ev_no) { event_nums.insert(ev_no); }
  void add(uint32_t, std::unique_ptr<to_GUI_t>);
  void add(std::unique_ptr<new_game_t>);

  // Smallest event number which wasn't received from the game server.
  uint32_t next_expected_event_no() const;

  // Send events not sent, up to the first hole in history.
  void send(const socket_t&);
  // There's nothing to send to GUI.
  bool empty() const;
};

class game_to_gui_t {
 private:
  game_t game;
  // Buffer for receiving messages from the game server.
  // + 1 is crucial for checking if datagram from the server is too large.
  std::array<byte_t, MAX_GAME_MSG_LEN + 1> from_game;
  // `receive` helper, receive a packet without complex processing
  ssize_t receive_packet(const socket_t& game_sock);
  // Once an event is parsed, handle it.
  void handle_event(event_type_t event_type, game_id_t game_id,
                    uint32_t event_no, const byte_t* data, size_t data_len);

 public:
  // Receive a message from the game server
  // and integrate it into the game_t queue.
  void receive(const socket_t& game_sock);
  // Send data to the GUI server.
  void send(const socket_t& gui_sockfd) { game.send(gui_sockfd); }
  // There's nothing to send to GUI.
  bool empty() const { return game.empty(); }

  uint32_t next_expected_event_no() const;
};
}  // namespace client

#endif /* FROM_GAME_H */
