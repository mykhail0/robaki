#ifndef GAME_H
#define GAME_H

#include "to_client.h"
#include "generator.h"
#include <vector>

struct worm_t {
    using float_t = long double;
    // using float_t = float;
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
    // Increments worm_t data for the iteration based on the turning speed.
    // Returns true iff stayed on the pixel.
    bool increment(int);

    position_t pos() const { return {(int32_t) x, (int32_t) y}; }
    void die() { alive = false; }
    bool is_alive() const { return alive; }
    bool is_negative() const { return x < 0 || y < 0; }
};

// Game data structure.
struct State {
    game_id_t game_id;

    int turning_speed;

    dim_t width;
    dim_t height;
    std::vector<std::vector<bool>> pixels;

    size_t dead;

    std::vector<Event> history;
    std::unique_ptr<RandGenerator> gen;
    size_t worms_num;
    // Sorted by owners in alphabetical order.
    std::vector<worm_t> worms;

    // Changes pixels.
    // Adds message to history.
    // Kills the worm if needed.
    void move_worm(int32_t, int32_t, size_t);

    bool init;

public:
    State(int, dim_t, dim_t, std::unique_ptr<RandGenerator>);

    void update_worm(size_t i, byte_t turn_dir) { worms[i].update(turn_dir); }
    // New round.
    size_t increment();
    // New game.
    size_t refresh(const Event &, const std::vector<byte_t> &);
    bool finished() const { return dead == worms_num - 1; }
    // true iff refresh() has been called.
    bool is_init() const { return init; }
};

#endif /* GAME_H */
