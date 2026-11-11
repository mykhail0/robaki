#ifndef TO_CLIENT_H
#define TO_CLIENT_H

#include <vector>

#include "../utility.h"

namespace server {
struct event_data_t {
  virtual byte_t event_type() const = 0;
  virtual size_t size() const = 0;
};

// Add `event_type` information.
struct event_t {
 private:
  uint32_t len;
  static constexpr size_t FIELD_LEN_BEFORE_EVENT_DATA =
      sizeof(uint32_t) /* len */ + sizeof(uint32_t) /* event_no */ +
      sizeof(byte_t) /* event_type */;
  std::vector<byte_t> bytes;

  class new_game_t : public event_data_t {
   private:
    // Length of concatenated player names string with '\0' delimiters.
    size_t list_len;

   public:
    new_game_t(size_t list_len) : list_len(list_len) {}
    byte_t event_type() const {
      return static_cast<byte_t>(event_type_t::new_game);
    }
    size_t size() const { return 2 * sizeof(dim_t) + list_len; }
  };

  class pixel_t : public event_data_t {
   public:
    byte_t event_type() const {
      return static_cast<byte_t>(event_type_t::pixel);
    }
    size_t size() const { return GAME_PIXEL_MSG_LEN; }
  };

  class player_eliminated_t : public event_data_t {
   public:
    byte_t event_type() const {
      return static_cast<byte_t>(event_type_t::player_eliminated);
    }
    size_t size() const { return GAME_PLAYERELIMINATED_MSG_LEN; }
  };

  class game_over_t : public event_data_t {
   public:
    byte_t event_type() const {
      return static_cast<byte_t>(event_type_t::game_over);
    }
    size_t size() const { return GAME_GAMEOVER_MSG_LEN; }
  };

  // Private initialize constructor to add values common for all events to the
  // packet.
  event_t(const event_data_t&);

  // Last method to be called inside constructor.
  void add_crc();

 public:
  // `NEW_GAME` event.
  event_t(dim_t, dim_t, const std::string&);
  // `PIXEL` event.
  event_t(byte_t, dim_t, dim_t);
  // `PLAYER_ELIMINATED` event.
  event_t(byte_t);
  // `GAME_OVER` event.
  event_t();

  // Almost `serializable` but stronger, since `memcpy` requires constness of
  // src.
  size_t size() const { return (sizeof len) + len + sizeof(uint32_t); }
  byte_t const* data() const { return bytes.data(); }

  // `event_no` is handled automatically with this,
  // without passing arguments to the ctor.
  // `reset_cnt` must be called whenever a new game starts.
  static uint32_t cnt;
  static void reset_cnt() { event_t::cnt = 0; }
};
}  // namespace server

#endif /* TO_CLIENT_H */
