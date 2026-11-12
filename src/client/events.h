#ifndef EVENTS_H
#define EVENTS_H

#include <vector>

#include "../socket.h"
#include "../utility.h"

namespace client {
// Interface for sending data to the GUI server.
class to_GUI_t : public serializable {
 protected:
  size_t off = 0;
  std::vector<byte_t> bytes;
  size_t size_;

  explicit to_GUI_t(size_t size) : size_(size) {}

 public:
  bool finished() const { return off == size_; }
  size_t size() const { return size_; }
  // Send data using a given socket.
  void send_to_GUI(const socket_t&);
};

// Below are a few classes used for parsing event_data portion of messages
// from the game server. Some of them also facilitate sending data to the
// GUI server. Those that do that manage memory dynamically through pointers,
// so that memory usage is minimal (memory for the message to be sent is
// allocated when needed, when an object is still in the queue,
// its memory pointer is nullptr).

class new_game_t : public to_GUI_t {
 private:
  dimensions_t dimensions;

 public:
  std::vector<std::string> player_name_list;

 private:
  void player_name_list_is_ok() const;

 public:
  new_game_t(const byte_t* to_parse, size_t len);

  dimensions_t get_dimensions() const { return dimensions; }
  // `serializable_to_GUI` interface implementation;
  const byte_t* data();
};

class pixel_t : public to_GUI_t {
 private:
  dimensions_t pos;
  std::string player;

 public:
  // The map is used to retrieve `player` field.
  pixel_t(const byte_t* to_parse, size_t len,
          const std::vector<std::string>& names,
          const dimensions_t& dimensions);

  // `serializable_to_GUI` interface implementation.
  const byte_t* data();
};

class player_eliminated_t : public to_GUI_t {
 private:
  std::string player;

 public:
  // The map is used to retrieve `player` field.
  player_eliminated_t(const byte_t* to_parse, size_t len,
                      const std::vector<std::string>& names);

  // `serializable_to_GUI` interface implementation.
  const byte_t* data();
};

// This one is not sent to the GUI server.
class game_over_t {
 public:
  game_over_t(size_t len);
};
}  // namespace client

#endif /* EVENTS_H */
