#ifndef EVENTS_H
#define EVENTS_H

#include <unordered_map>
#include <vector>

#include "../socket.h"
#include "../utility.h"

// TODO take care of new and delete, this shouldnt be used in modern C++

namespace client {
// Interface for sending data to the GUI server.
class to_GUI_t : public serializable {
 protected:
  size_t off;
  mutable byte_t* bytes;
  size_t sz;

  to_GUI_t(size_t sz) : off(0), bytes(nullptr), sz(sz) {}

 public:
  // Offset points to the position in bytes where to start sending.
  // Is needed because of the TCP nature of the connection.
  void offset(size_t x) { off += x; }
  bool finished() const { return off == sz; }
  size_t size() const { return sz; }
  // Sends itself via a given socket.
  // Returns true if everything was sent successfully.
  // Returns false if there is still something to be sent
  // but the socket would block. Otherwise exits(1).
  bool send_to_GUI(const socket_t&);
};

// Below are a few classes used for parsing event_data portion of messages
// from the game server. Some of them also facilitate sending data to the
// GUI server. Those that do that manage memory dynamically through pointers,
// so that memory usage is minimal (memory for the message to be sent is
// allocated when needed, when an object is still in the queue,
// its memory pointer is nullptr).

class new_game_t : public to_GUI_t {
 private:
  dim_t maxx;
  dim_t maxy;
  std::vector<std::string> player_name_list;

 public:
  new_game_t(byte_t*, size_t);

  // serializable_to_GUI interface implementation;
  byte_t const* data() const;

  ~new_game_t() { delete bytes; }

  std::vector<std::string> get_names() const { return player_name_list; }
};

class pixel_t : public to_GUI_t {
 private:
  dim_t x;
  dim_t y;
  std::string player;

 public:
  // The map is used to retrieve `player` field.
  pixel_t(byte_t*, size_t, const std::unordered_map<byte_t, std::string>&);

  // serializable_to_GUI interface implementation.
  byte_t const* data() const;

  ~pixel_t() { delete bytes; }
};

class player_eliminated_t : public to_GUI_t {
 private:
  std::string player;

 public:
  // The map is used to retrieve `player` field.
  player_eliminated_t(byte_t*, size_t,
                      const std::unordered_map<byte_t, std::string>&);

  // serializable_to_GUI interface implementation.
  byte_t const* data() const;

  ~player_eliminated_t() { delete bytes; }
};

// This one is not sent to the GUI server.
class game_over_t {
 public:
  game_over_t(size_t);
};
}  // namespace client

#endif /* EVENTS_H */
