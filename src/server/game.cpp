#include "game.h"

#include <cmath>

server::worm_t::float_t server::worm_t::PI = 3.141592653589793L;

server::worm_t::worm_t(dimensions_t rand_pos, uint32_t rand_direction,
                       dimensions_t dimensions, turn_direction_t direction)
    : alive(true) {
  last_turn = direction;
  x = rand_pos.x % dimensions.x + (float_t)0.5L;
  y = rand_pos.y % dimensions.y + (float_t)0.5L;

  this->direction = rand_direction % 360;
}

bool server::worm_t::increment(uint8_t turning_speed) {
  if (last_turn != turn_direction_t::straight) {
    if (last_turn == turn_direction_t::right) {
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

server::state_t::state_t(int turning_speed, dimensions_t dimensions,
                         std::unique_ptr<RNG> gen)
    : turning_speed(turning_speed),
      dimensions(dimensions),
      pixels(dimensions.y, std::vector<bool>(dimensions.x, false)),
      gen(std::move(gen)) {}

void server::state_t::update_worm(size_t i, turn_direction_t turn_dir) {
  worms[i].update(turn_dir);
}

void server::state_t::move_worm(size_t i) {
  dimensions_t pos = worms[i].pos();
  if (worms[i].is_negative() or not dimensions.contains(pos) or
      pixels[pos.y][pos.x]) {
    worms[i].die();
    ++dead;
    history.emplace_back(i);
    if (finished()) {
      history.emplace_back();
    }
  } else {
    pixels[pos.y][pos.x] = true;
    history.emplace_back(i, pos);
  }
}

void server::state_t::refresh(event_t&& new_game,
                              const std::vector<turn_direction_t>& directions) {
  dead = 0;
  game_id = gen->generate();
  worms.clear();
  history.clear();
  history.emplace_back(std::move(new_game));
  for (auto& x : pixels) {
    for (auto&& e : x) {
      e = false;
    }
  }
  size_t worms_num = directions.size();
  for (size_t i = 0; i < worms_num and dead + 1 != worms_num; ++i) {
    dimensions_t pos;
    pos.x = gen->generate();
    pos.y = gen->generate();
    worms.emplace_back(pos, gen->generate(), dimensions, directions[i]);
    move_worm(i);
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
      move_worm(i);
    }
  }
  return first_inserted;
}
