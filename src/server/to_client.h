#ifndef TO_CLIENT_H
#define TO_CLIENT_H

#include <array>
#include <stack>

#include "../utility.h"

namespace server {
struct event_data_t : virtual public serializable {
  virtual byte_t event_type() const = 0;
};

class new_game_t : public event_data_t {
 private:
  std::array<byte_t, MAX_GAME_NEWGAME_MSG_LEN> bytes;
  // Length of concatenated player names string with '\0' delimiters.
  size_t list_len;

 public:
  new_game_t(dim_t, dim_t, size_t, const char*);

  // EventData interface implementation.
  byte_t event_type() const { return NEW_GAME; }
  byte_t const* data() const { return bytes.data(); }
  size_t size() const { return 2 * sizeof(dim_t) + list_len; }
};

class pixel_t : public event_data_t {
 private:
  std::array<byte_t, GAME_PIXEL_MSG_LEN> bytes;

 public:
  pixel_t(byte_t, dim_t, dim_t);

  // EventData interface implementation.
  byte_t event_type() const { return PIXEL; }
  byte_t const* data() const { return bytes.data(); }
  size_t size() const { return GAME_PIXEL_MSG_LEN; }
};

class player_eliminated_t : public event_data_t {
 private:
  std::array<byte_t, GAME_PLAYERELIMINATED_MSG_LEN> bytes;

 public:
  player_eliminated_t(byte_t player) { bytes[0] = player; }

  // EventData interface implementation.
  byte_t event_type() const { return PLAYER_ELIMINATED; }
  byte_t const* data() const { return bytes.data(); };
  size_t size() const { return GAME_PLAYERELIMINATED_MSG_LEN; }
};

class game_over_t : public event_data_t {
  // No data is present.

 public:
  // EventData interface implementation.
  byte_t event_type() const { return GAME_OVER; }
  byte_t const* data() const { return nullptr; }
  size_t size() const { return GAME_GAMEOVER_MSG_LEN; }
};

// Add `event_type` information.
struct event_t : public serializable {
  size_t sz;
  std::array<byte_t, MAX_GAME_EVENT_MSG_LEN> bytes;

 public:
  event_t(const event_data_t&);

  // Serializable interface implementation.
  size_t size() const { return sz; }
  byte_t const* data() const { return bytes.data(); }

  // event_no is handled automatically with this,
  // without passing arguments to the ctor.
  // `reset_cnt` must be called whenever a new game starts.
  static uint32_t cnt;
  static void reset_cnt() { event_t::cnt = 0; }
};
}  // namespace server

#endif /* TO_CLIENT_H */
