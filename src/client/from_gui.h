#ifndef FROM_GUI_H
#define FROM_GUI_H

#include <string>
#include <unordered_map>

#include "../consts.h"
#include "../socket.h"

namespace client {
class from_GUI_t {
 private:
  byte_t turn_dir;

  // Buffer for receiving.
  char buf[1000];
  // string for parsing purposes (insted of char[]).
  // Contains characters received from the GUI server
  // after the most recently received new line character.
  std::string str;

  // Meaning of these maps is clear in the ctor.
  std::unordered_map<size_t, std::string> msg;
  std::unordered_map<size_t, byte_t> turn;
  std::unordered_map<size_t, bool> down;

 public:
  from_GUI_t();
  // Receives data from the given socket
  // until the socket would block (EWOULDBLOCK or EAGAIN).
  // Adds received data to the `str`.
  void receive_data(const socket_t&);
  // Receives data from the given socket
  // until the socket would block (EWOULDBLOCK or EAGAIN).
  // Updates turn_dir accordingly.
  void receive(const socket_t&);
  byte_t turn_direction() const { return turn_dir; }
};
}  // namespace client

#endif /* FROM_GUI_H */
