#ifndef TO_CLIENT_H
#define TO_CLIENT_H

#include "../utility.h"
#include <stack>

struct EventData : virtual public Serializable {
    virtual byte_t event_type() const = 0;
};

class NewGame : public EventData {
    private:
    byte_t bytes[MAX_GAME_NEWGAME_MSG_LEN];
    // Length of concatenated player names string w spaces.
    size_t list_len;

    public:
    NewGame(dim_t, dim_t, size_t, const char *);

    // EventData interface implementation.
    byte_t event_type() const { return NEW_GAME; }
    byte_t const *data() const { return bytes; }
    size_t size() const { return 8 + list_len; }
};

class Pixel : public EventData {
    private:
    byte_t bytes[GAME_PIXEL_MSG_LEN];

    public:
    Pixel(byte_t, dim_t, dim_t);

    // EventData interface implementation.
    byte_t event_type() const { return PIXEL; }
    byte_t const *data() const { return bytes; }
    size_t size() const { return GAME_PIXEL_MSG_LEN; }
};

class PlayerEliminated : public EventData {
    private:
    byte_t bytes[GAME_PLAYERELIMINATED_MSG_LEN];

    public:
    PlayerEliminated(byte_t player) { bytes[0] = player; }

    // EventData interface implementation.
    byte_t event_type() const { return PLAYER_ELIMINATED; }
    byte_t const *data() const { return bytes; };
    size_t size() const { return GAME_PLAYERELIMINATED_MSG_LEN; }
};

class GameOver : public EventData {
    // No data is present.

    public:
    // EventData interface implementation.
    byte_t event_type() const { return GAME_OVER; }
    byte_t const *data() const { return nullptr; }
    size_t size() const { return GAME_GAMEOVER_MSG_LEN; }
};

// Adds event type information.
struct Event : public Serializable {
    size_t sz;
    byte_t bytes[MAX_GAME_EVENT_MSG_LEN];

    public:
    Event(const EventData &);

    // Serializable interface implementation.
    size_t size() const { return sz; }
    byte_t const *data() const { return bytes; }

    /* event_no is handled automatically with this,
    without passing arguments to the ctor.
    reset_cnt() must be called whenever a new game starts. */
    static uint32_t cnt;
    static void reset_cnt() { Event::cnt = 0; }
};

#endif /* TO_CLIENT */
