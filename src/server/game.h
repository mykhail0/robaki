#ifndef GAME_H
#define GAME_H

#include <iostream>
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
  // Increment worm_t data for the iteration based on the turning speed.
  // Return `true` iff stayed on the pixel.
  bool increment(int);

  position_t pos() const { return {(int32_t)x, (int32_t)y}; }
  void die() { alive = false; }
  bool is_alive() const { return alive; }
  bool is_negative() const { return x < 0 || y < 0; }
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
  size_t worms_num;
  // Sorted by owners in alphabetical order.
  std::vector<worm_t> worms;

  // Change pixels.
  // Add message to history.
  // Kill the worm if needed.
  void move_worm(int32_t, int32_t, size_t);

  bool init;

 public:
  state_t(int, dim_t, dim_t, std::unique_ptr<RNG>);

  void update_worm(size_t i, byte_t turn_dir) { worms[i].update(turn_dir); }
  // Two functions below return the number of events in the game.

  // New round. This function also returns the index of the first inserted event
  // in this round. If nothing was inserted, this index is negative.
  std::pair<ssize_t, size_t> increment();
  // New game.
  size_t refresh(const event_t&, const std::vector<byte_t>&);
  bool finished() const { return dead == worms_num - 1; }
  // `true` iff `refresh()` has been called.
  bool is_init() const { return init; }

  void print() const {
    std::cerr << "game events: " << history.size() << std::endl;
  }
};
}  // namespace server

#endif /* GAME_H */
