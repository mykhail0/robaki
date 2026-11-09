#ifndef GAME_H
#define GAME_H

#include <optional>
#include <vector>

#include "rng.h"
#include "to_client.h"

namespace server {
struct worm_t {
  using float_t = long double;
  byte_t last_turn;
  uint16_t direction;
  float_t x;
  float_t y;

  bool alive;

  static float_t PI;

  using position_t = std::pair<int32_t, int32_t>;

 public:
  worm_t(uint32_t, uint32_t, uint32_t, dim_t, dim_t, byte_t);

  void update(byte_t turn_dir) { last_turn = turn_dir; }
  // Increment `worm_t` data for the iteration based on the turning speed.
  // Return `true` iff stayed on the same pixel.
  bool increment(uint8_t turning_speed);

  position_t pos() const { return {(int32_t)x, (int32_t)y}; }
  void die() { alive = false; }
  bool is_alive() const { return alive; }
  bool is_negative() const { return x < 0 or y < 0; }
};

// Game data structure.
struct state_t {
  game_id_t game_id;

  int turning_speed;

  dim_t width;
  dim_t height;
  std::vector<std::vector<bool>> pixels;

  size_t dead;

  std::vector<event_t> history;
  std::unique_ptr<RNG> gen;
  // Sorted by owners in alphabetical order.
  std::vector<worm_t> worms;

  // Change pixels. Add message to history. Kill the worm if needed.
  void move_worm(int32_t, int32_t, size_t);

 public:
  state_t(int, dim_t, dim_t, std::unique_ptr<RNG>);

  void update_worm(size_t i, byte_t turn_dir) { worms[i].update(turn_dir); }

  // New round. Return the index of the first inserted event
  // in this round. If nothing was inserted, return nothing.
  std::optional<uint32_t> play_round();
  // Refresh the game state for a new game with given starter directions.
  void refresh(event_t&& new_game, const std::vector<byte_t>& directions);
  bool finished() const { return dead == worms.size() - 1; }
};
}  // namespace server

#endif /* GAME_H */
