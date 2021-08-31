#ifndef FROM_GAME_H
#define FROM_GAME_H

#include "iset.h"
#include "events.h"
#include <memory>
#include <queue>
#include <map>
#include <unordered_set>

// A class responsible for managing receiving messages from the game server and
// sending them to the GUI server in the context of a single game.
// So called event buffer.
class Game {
    game_id_t game_id;

    // Interval set is used for
    // efficiently extracting the next_expected_event_no.
    iset event_nums;
    // The smallest event number which still wasn't sent to the GUI server.
    uint32_t smallest_not_sent;
    // Events still not sent to the GUI server. Keys are event numbers.
    std::map<uint32_t, std::unique_ptr<Serializable2GUI>> received;

    // The NEW_GAME event.
    NewGame ev0;

    // See bool Game::is_sending_the_rest() const;
    bool sending_the_rest;

public:
    Game(game_id_t, const NewGame &);

    game_id_t id() const { return game_id; }
    // Adds event to the event buffer.
    void add(uint32_t, std::unique_ptr<Serializable2GUI>);

    // next_expected_event_no for this game.
    // Smallest event number which wasn't received from the game server.
    uint32_t expected() const { return event_nums.smallest_not_included(); }

    // 2 functions below return true if everything was sent successfully.
    // Return false if there is still something to be sent
    // but the socket would block. Otherwise exit(1).
    // Receive a socket to send data to.

    // Sends events not sent, up to the first hole in history.
    bool send(int);
    // Sends all events not sent.
    bool send_the_rest(int);

    // Returns true iff send_the_rest() was already called.
    bool is_sending_the_rest() const { return sending_the_rest; }
    // Numeration of players for this game.
    std::unordered_map<byte_t, std::string> names;
};

class Game2GUI {
    private:
    // Queue of game buffers.
    // Received messages can influence only the q.back().
    // Other elements are there to be sent to the GUI server.
    std::queue<Game> q;
    // See ctor.
    std::unordered_set<byte_t> known_event_types;
    // Buffer for receiving messages from the game server.
    byte_t from_game[MAX_GAME_MSG_LEN];

    public:
    Game2GUI() : known_event_types({NEW_GAME, PIXEL, PLAYER_ELIMINATED, GAME_OVER}) {}

    bool event_is_known(byte_t) const;

    // Receives a message from a given socket fd (of the game server)
    // and integrates it into the <Game> queue.
    void recv(int, const SockAddr &);
    // Sends data to the GUI server.
    // Returns false if there is still something to be sent
    // but the socket would block. Otherwise exits(1).
    // Receives a socket to send data to.
    bool send(int);

    // next_expected_event_no
    uint32_t expected() const { return q.empty() ? 0 : q.back().expected(); }
};

#endif /* FROM_GAME_H */
