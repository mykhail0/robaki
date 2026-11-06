#include "game.h"

#include <cmath>

server::worm_t::float_t server::worm_t::PI = 3.141592653589793L;

server::worm_t::worm_t(uint32_t randX, uint32_t randY, uint32_t randDir,
                       dim_t width, dim_t height, byte_t dir)
    : alive(true) {
  last_turn = dir;
  x = randX % width + (float_t)0.5L;
  y = randY % height + (float_t)0.5L;

  direction = randDir % 360;
}

bool server::worm_t::increment(uint8_t turning_speed) {
  if (last_turn != STRAIGHT) {
    if (last_turn == RIGHT) {
      direction += turning_speed;
      direction %= 360;
    } else {
      direction += ((direction < turning_speed) ? 360 : 0);
      direction -= turning_speed;
    }
  }
  float_t angle = worm_t::PI * direction / 180;
  float_t dx = cosl(angle), dy = sinl(angle);
  x += dx;
  y += dy;
  return ((int32_t)x == (int32_t)(x - dx)) and
         ((int32_t)y == (int32_t)(y - dy));
}

server::state_t::state_t(int turning_speed, dim_t width, dim_t height,
                         std::unique_ptr<RNG> gen)
    : turning_speed(turning_speed),
      width(width),
      height(height),
      pixels(height, std::vector<bool>(width, false)),
      gen(std::move(gen)) {}

void server::state_t::move_worm(int32_t x, int32_t y, size_t i) {
  if (worms[i].is_negative() or x < 0 or y < 0 or width <= (dim_t)x or
      height <= (dim_t)y or pixels[y][x]) {
    worms[i].die();
    ++dead;
    history.emplace_back(i);
    if (finished()) {
      history.emplace_back();
    }
  } else {
    pixels[y][x] = true;
    history.emplace_back(i, x, y);
  }
}

void server::state_t::refresh(const event_t& new_game,
                              const std::vector<byte_t>& directions) {
  dead = 0;
  game_id = gen->generate();
  worms.clear();
  history.clear();
  history.push_back(new_game);
  for (auto& x : pixels) {
    for (auto&& e : x) {
      e = false;
    }
  }
  size_t worms_num = directions.size();
  for (size_t i = 0; i < worms_num and dead + 1 != worms_num; ++i) {
    auto x = gen->generate();
    auto y = gen->generate();
    worms.emplace_back(x, y, gen->generate(), width, height, directions[i]);
    const auto [X, Y] = worms.back().pos();
    move_worm(X, Y, i);
  }
}

std::optional<uint32_t> server::state_t::play_round() {
  std::optional<uint32_t> first_inserted{};
  for (size_t i = 0; i < worms.size() and not finished(); ++i) {
    if (worms[i].is_alive() and not worms[i].increment(turning_speed)) {
      // Did not stay on the same pixel.
      if (not first_inserted.has_value()) {
        first_inserted = history.size();
      }
      const auto [x, y] = worms[i].pos();
      move_worm(x, y, i);
    }
  }
  return first_inserted;
}
