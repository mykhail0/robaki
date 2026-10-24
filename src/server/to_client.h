#ifndef TO_CLIENT_H
#define TO_CLIENT_H

#include <array>
#include <stack>

#include "../utility.h"

struct EventData : virtual public Serializable {
  virtual byte_t event_type() const = 0;
};

class NewGame : public EventData {
 private:
  std::array<byte_t, MAX_GAME_NEWGAME_MSG_LEN> bytes;
  // Length of concatenated player names string with '\0' delimiters.
  size_t list_len;

 public:
  NewGame(dim_t, dim_t, size_t, const char*);

  // EventData interface implementation.
  byte_t event_type() const { return NEW_GAME; }
  byte_t const* data() const { return bytes.data(); }
  size_t size() const { return 2 * sizeof(dim_t) + list_len; }
};

class Pixel : public EventData {
 private:
  std::array<byte_t, GAME_PIXEL_MSG_LEN> bytes;

 public:
  Pixel(byte_t, dim_t, dim_t);

  // EventData interface implementation.
  byte_t event_type() const { return PIXEL; }
  byte_t const* data() const { return bytes.data(); }
  size_t size() const { return GAME_PIXEL_MSG_LEN; }
};

class PlayerEliminated : public EventData {
 private:
  std::array<byte_t, GAME_PLAYERELIMINATED_MSG_LEN> bytes;

 public:
  PlayerEliminated(byte_t player) { bytes[0] = player; }

  // EventData interface implementation.
  byte_t event_type() const { return PLAYER_ELIMINATED; }
  byte_t const* data() const { return bytes.data(); };
  size_t size() const { return GAME_PLAYERELIMINATED_MSG_LEN; }
};

class GameOver : public EventData {
  // No data is present.

 public:
  // EventData interface implementation.
  byte_t event_type() const { return GAME_OVER; }
  byte_t const* data() const { return nullptr; }
  size_t size() const { return GAME_GAMEOVER_MSG_LEN; }
};

// Add `event_type` information.
struct Event : public Serializable {
  size_t sz;
  std::array<byte_t, MAX_GAME_EVENT_MSG_LEN> bytes;

 public:
  Event(const EventData&);

  // Serializable interface implementation.
  size_t size() const { return sz; }
  byte_t const* data() const { return bytes.data(); }

  // event_no is handled automatically with this,
  // without passing arguments to the ctor.
  // `reset_cnt` must be called whenever a new game starts.
  static uint32_t cnt;
  static void reset_cnt() { Event::cnt = 0; }
};

#endif /* TO_CLIENT_H */
