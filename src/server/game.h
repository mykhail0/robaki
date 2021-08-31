#ifndef GAME_H
#define GAME_H

#include "to_client.h"
#include "generator.h"
#include <vector>

struct worm_t {
    byte_t last_turn;
    uint16_t direction;
    long double x;
    long double y;

    bool alive;

    static long double PI;

    using position_t = std::pair<int32_t, int32_t>;

public:
    worm_t(uint32_t, uint32_t, uint32_t, dim_t, dim_t);

    void update(byte_t turn_dir) { last_turn = turn_dir; }
    // Increments worm_t data for the iteration based on the turning speed.
    // Returns true iff stayed on the pixel.
    bool increment(int);

    position_t pos() const { return {(int32_t) x, (int32_t) y}; }
    void die() { alive = false; }
    bool is_alive() { return alive; }
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

public:
    State(int, dim_t, dim_t, std::unique_ptr<RandGenerator>);

    void update_worm(size_t i, byte_t turn_dir) { worms[i].update(turn_dir); }
    // New round.
    size_t increment();
    // New game.
    size_t refresh(const Event &, size_t);
    bool finished() const { return dead == worms_num - 1; }
};

#endif /* GAME_H */
