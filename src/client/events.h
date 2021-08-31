#ifndef EVENTS_H
#define EVENTS_H

#include "../utility.h"
#include <vector>
#include <unordered_map>

// Interface for sending data to the GUI server.
class Serializable2GUI : public Serializable {
protected:
    size_t off;
    mutable byte_t *bytes;
    size_t sz;

    Serializable2GUI(size_t sz) : off(0), bytes(nullptr), sz(sz) {}

public:
    // Offset points to the position in bytes where to start sending.
    // Is needed because of the TCP nature of the connection.
    void offset(size_t x) { off += x; }
    bool finished() const { return off == sz; }
    size_t size() const { return sz; }
    // Sends itself via a given socket.
    // Returns true if everything was sent successfully.
    // Returns false if there is still something to be sent
    // but the socket would block. Otherwise exits(1).
    bool send2GUI(int);
};

// Below are a few classes used for parsing event_data portion of messages
// from the game server. Some of them also facilitate sending data to the
// GUI server. Those that do that manage memory dynamically through pointers,
// so that memory usage is minimal (memory for the message to be sent is
// allocated when needed, when an object is still in the queue,
// its memory pointer is nullptr).

class NewGame : public Serializable2GUI {
    private:
    dim_t maxx;
    dim_t maxy;
    std::vector<std::string> player_name_list;

    public:
    NewGame(byte_t *, size_t);

    // Serializable 2GUI interface implementation;
    byte_t const *data() const;

    ~NewGame() { delete bytes; }

    std::vector<std::string> get_names() const { return player_name_list; }
};

class Pixel : public Serializable2GUI {
    private:
    dim_t x;
    dim_t y;
    std::string player;

    public:
    // The map is used to retrieve `player` field.
    Pixel(byte_t *, size_t, const std::unordered_map<byte_t, std::string> &);

    // Serializable2GUI interface implementation.
    byte_t const *data() const;

    ~Pixel() { delete bytes; }
};

class PlayerEliminated : public Serializable2GUI {
    private:
    std::string player;

    public:
    // The map is used to retrieve `player` field.
    PlayerEliminated(byte_t *, size_t, const std::unordered_map<byte_t, std::string> &);

    // Serializable2GUI interface implementation.
    byte_t const *data() const;

    ~PlayerEliminated() { delete bytes; }
};

// This one is not sent to the GUI server.
class GameOver {
    public:
    GameOver(size_t);
};

#endif /* EVENTS_H */
