#ifndef FROM_GUI_H
#define FROM_GUI_H

#include <array>
#include <string>

#include "../consts.h"
#include "../socket.h"

namespace client {
class from_GUI_t {
 private:
  bool left_down = false;
  bool right_down = false;
  bool last_pressed_left = true;

  // Buffer for receiving.
  std::array<byte_t, 1000> buf;
  // string for parsing purposes
  // Contains characters received from the GUI server
  // after the most recently received new line character.
  std::string str;

  // Parse messages using their lengths.
  static constexpr size_t GUI_MSG_TYPES = 4;
  static size_t len_to_ind(size_t len) { return len % GUI_MSG_TYPES; }
  std::array<std::string, GUI_MSG_TYPES> msg;
  std::array<turn_direction_t, GUI_MSG_TYPES> turn;
  std::array<bool, GUI_MSG_TYPES> down;

 public:
  from_GUI_t();
  // Receive data from the given socket
  // until the socket would block (EWOULDBLOCK or EAGAIN).
  // Append received data to the `str`.
  void receive_data(const socket_t&);
  // Receive data from the given socket
  // until the socket would block (EWOULDBLOCK or EAGAIN).
  // Update `turn_dir` accordingly.
  void receive(const socket_t&);
  byte_t turn_direction() const;
};
}  // namespace client

#endif /* FROM_GUI_H */
