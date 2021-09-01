#ifndef SERVER_H
#define SERVER_H

#include "game.h"
#include <queue>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <chrono>

// Declaration, for int Server2Client::send(const Server &);
class Server;

using clientkey_t = std::pair<SockAddr, session_id_t>;

/* Message object to facilitate sending events information to clients.
Used in 2 cases. To send everyone events generated in the round.
Or to send a player every event up to the current one.
In this case end needs to be equal -1. */
class Server2Client {
    std::stack<clientkey_t> recipients;
    ssize_t ev_no;
    ssize_t start;
    ssize_t end;

    public:
    // Receives legit start, end and recipients.
    Server2Client(const std::stack<clientkey_t> &, ssize_t, ssize_t);
    // Receives a single recipient and the start.
    Server2Client(const clientkey_t &, ssize_t);

    // Lumps together as much events as possible for a datagram
    // (from ev_no to as far as the `end`).
    // Then sends it to the currently processed recipient.
    // When the recipient from the top of the stack receives everything,
    // it gets popped.
    // Returns similarly as sendto().
    int send(const Server &);
    // Returns true iff there is nothing more to send for this message.
    bool done() const { return recipients.empty(); }
};

struct Client {
    session_id_t id;
    /* When this deadline is older than chrono::now(),
    this Client should become invalid. */
    std::chrono::time_point<std::chrono::system_clock> deadline;
    std::string player_name;

    Client(session_id_t, const std::string &);
    /* This function should be called
    whenever a datagram from the client is received. */
    void update_deadline();
    // bool operator<(const Client &x) const { return player_name < x.player_name;}
};

// Values for `names` map in wait_for_start() phase.
constexpr int NAMES_DEFAULT = -1, NAMES_READY = 1;

// Helper struct.
// A part of Server::receive() return type.
// Holds useful information after the basic processing of a client's message.
struct received_t {
    byte_t turn_direction;
    std::string player_name;
    std::string removed;

    received_t() : turn_direction(0) {}
};

struct Server {
    int sockfd;
    byte_t buffer[MAX_2_GAME_MSG_LEN + 1];

    int rounds_per_sec;

    std::map<SockAddr, Client> clients;
    // Different roles while waiting for a game to start and while running a game.
    // Never contains an empty string as a key.
    // Default uninitialized value is NAMES_DEFUALT.
    std::unordered_map<std::string, int> names;
    // Queue of message objects for sending messages to clients.
    // (advanced msg queue)
    std::queue<Server2Client> q;
    State game;

    // Returns true iff the client is already associated with the server.
    bool contains(const clientkey_t &) const;
    // Adds a client.
    void add(const SockAddr &, session_id_t, const std::string &);
    // Removes a client.
    void remove(const SockAddr &);
    // Updates a client's deadline.
    void update(const SockAddr &);

    // Adds a single type message to the queue for events from
    // the specified point up to the end of the game history.
    void add_msg(const clientkey_t &, uint32_t);
    // Adds a broadcast type message to the queue for events from
    // the specified point up to the end of the game history.
    void add_broadcast_msg(uint32_t);

    // Helper function with mutual logic for wait_receive() and run_receive().
    std::pair<int, received_t> receive();

public:
    Server(int, int, dim_t, dim_t, uint32_t, int);
    // Called in wait_for_start(). Receives and processes a single message.
    // Modifies the set of ready players accordingly,
    // which is passed as an argument.
    void wait_receive(std::unordered_set<std::string> &);
    void run_receive();

    // Sends a single datagram, using `q`.
    void send();
    // Basically tells if there is anything to be sent.
    bool Qempty() { return q.empty(); }

    // Check for idle clients and remove those which are idle.
    // Called in wait_for_start().
    // As argument receives the set of ready players.
    void check_for_idle_clients(std::unordered_set<std::string> &);
    // Check for idle clients and remove those which are idle.
    // Called in run().
    void check_for_idle_clients();

    // Updates the game state. Called every round of the game.
    void update_game();

    // Returns true iff the game is over.
    bool done() const { return game.finished(); }

    // Prepares the server for run().
    void game_ready();
    // Prepares the server for wait_for_start().
    void game_over();

    int get_rounds_per_sec() const { return rounds_per_sec; }

    // Returns true iff a game has been played on the server.
    bool was_played() const { return game.is_init(); }

    void print() const { game.print(); }
};

#endif /* SERVER_H */
