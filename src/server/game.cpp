#include "game.h"
#include <cmath>

long double worm_t::PI = 3.14159265358979323846;

worm_t::worm_t(uint32_t randX, uint32_t randY, uint32_t randDir,
    dim_t width, dim_t height) : alive(true) {
    last_turn = STRAIGHT;
    x = randX % (width - 1) + (long double) 0.5L;
    y = randY % (height - 1) + (long double) 0.5L;
    direction = randDir % 360;
}

bool worm_t::increment(int turning_speed) {
    if (last_turn != STRAIGHT) {
        if (last_turn == RIGHT) {
            direction += turning_speed;
            direction %= 360;
        } else {
            direction += direction < turning_speed ? 360 : 0;
            direction -= turning_speed;
        }
    }
    long double angle = worm_t::PI * (360 - direction) / 180;
    auto dx = cos(angle),
        dy = - sin(angle);
    x += dx;
    y += dy;
    return ((int32_t) x == (int32_t) (x - dx)) &&
        ((int32_t) y == (int32_t) (y - dy));
}

State::State(int turning_speed, dim_t width, dim_t height,
    std::unique_ptr<RandGenerator> gen) :
turning_speed(turning_speed), width(width), height(height),
pixels(height, std::vector<bool>(width, false)), gen(std::move(gen)) {}

void State::move_worm(int32_t x, int32_t y, size_t i) {
    if (((x < 0 || y < 0) || (width <= (dim_t) x || height <= (dim_t) y)) ||
        pixels[y][x]) {
        worms[i].die();
        ++dead;
        history.push_back(Event(PlayerEliminated(i)));
        if (finished())
            history.push_back(Event(GameOver()));
    } else {
        pixels[y][x] = true;
        history.push_back(Event(Pixel(i, x, y)));
    }
}

size_t State::refresh(const Event &new_game, size_t w) {
    worms_num = w;
    dead = 0;
    game_id = gen->generate();
    worms.clear();
    history.clear();
    history.push_back(new_game);
    for (auto &x: pixels) {
        for (auto &&e: x)
            e = false;
    }
    for (size_t i = 0; i < worms_num && not finished(); ++i) {
        auto x = gen->generate();
        auto y = gen->generate();
        worms.push_back(worm_t(x, y, gen->generate(), width, height));
        move_worm(x, y, i);
    }

    return history.size();
}

size_t State::increment() {
    for (size_t i = 0; i < worms.size() && not finished(); ++i) {
        if (not worms[i].is_alive())
            continue;
        if (not worms[i].increment(turning_speed)) {
            // Did not stay on the same pixel.
            auto [x, y] = worms[i].pos();
            move_worm(x, y, i);
        }
    }

    return history.size();
}