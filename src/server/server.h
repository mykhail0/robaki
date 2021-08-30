#include "../consts.h"

#include <netinet/in.h>

#include <vector>
#include <queue>
#include <chrono>

using clientkey_t = std::pair<SockAddr, session_id_t>;

class EventData : virtual public Serializable {
    virtual byte_t event_type() = 0;
}

class NewGame : public EventData {
    private:
    byte_t bytes[MAX_SRVR_NEWGAME_MSG_LEN];
    // Length of concatenated player names string w spaces.
    size_t list_len;

    public:
    NewGame(dim_t, dim_t, size_t, char *);

    // EventData interface implementation.
    byte_t event_type() { return NEW_GAME; }

    // Serializable interface implementation.
    byte_t *data() { return bytes; }
    size_t size() const { return 8 + list_len; }
};

class Pixel : public EventData {
    private:
    byte_t bytes[SRVR_PIXEL_MSG_LEN];

    public:
    Pixel(byte_t, dim_t, dim_t);

    // EventData interface implementation.
    byte_t event_type() { return PIXEL; }

    // Serializable interface implementation.
    byte_t *data() { return bytes; }
    size_t size() const { return SRVR_PIXEL_MSG_LEN; }
};

class PlayerEliminated : public EventData {
    private:
    byte_t bytes[SRVR_PLAYERELIMINATED_MSG_LEN];

    public:
    PlayerEliminated(byte_t player) { bytes[0] = player; }

    // EventData interface implementation.
    byte_t event_type() { return PLAYER_ELIMINATED; }

    // Serializable interface implementation.
    byte_t *data() { return bytes; };
    size_t size() const { return SRVR_PLAYERELIMINATED_MSG_LEN; }
};

class GameOver : public EventData {
    // No data is present.

    public:
    // EventData interface implementation.
    byte_t event_type() { return GAME_OVER; }

    // Serializable interface implementation.
    byte_t *data() { return nullptr; }
    size_t size() const { return SRVR_GAMEOVER_MSG_LEN; }
};

// Adds event type information.
struct Event : public Serializable {
    size_t datagram_sz;
    byte_t bytes[MAX_SRVR_EVENT_MSG_LEN];

    public:
    Event(EventData &);

    // Serializable interface implementation.
    size_t size() const { return datagram_sz; }
    byte_t *data() { return bytes; }

    /* With this event_no is handled automatically,
    without passing arguments to the ctor.
    reset_cnt() must be called whenever a new game starts. */
    static uint32_t cnt;
    static void reset_cnt() { Event::cnt = 0; }
};

// Declaration, for Server2Client::send()
class Server;

/* Used in 2 cases. To send everyone events generated in the round.
Or to send a player every event up to the current one.
In this case end needs to be equal -1. */
class Server2Client {
    std::stack<clientkey_t> recipients;
    size_t ev_no;
    size_t start;
    size_t end;

    public:
    Server2Client(const std::stack<clientkey_t> &recipients, size_t start, size_t end) : recipients(recipients), ev_no(start), start(ev_no), end(end) {}
    Server2Client(const clientkey_t &recipient, size_t ev_no) : ev_no(ev_no), start(ev_no), end(-1) { recipients.push(recipient); }

    int send(const Server &);
    bool done() { return recipients.empty(); }
};

struct Client2Server {
    session_id_t session_id;
    byte_t turn_direction;
    uint32_t next_expected_event_no;
    std::string player_name;

    Client2Server(byte_t *, size_t);
};

/* Some gaming data. */
struct worm_t {
    byte_t last_turn;
    uint16_t direction;
    long double x;
    long double y;

    static long double PI;

    /* Kind of reoccuring ctor.
    Default ctor is useless for initializing the struct. */
    void refresh(uint32_t, uint32_t, uint32_t);

    // Increments worm_t data for the iteration.
    // Returns true iff stayed on the pixel.
    bool increment();
};

struct Client {
    session_id_t id;
    /* When this deadline is older than chrono::now(),
    this Client should become invalid. */
    std::chrono::time_point<std::chrono::system_clock> deadline;
    std::string player_name;

    Client(session_id_t id, const std::string &player_name) :
        id(id), deadline(std::chrono::system_clock::now() + std::chrono::milliseconds {2000}), player_name(player_name) {}

    /* This function should be called
    whenever a datagram from the client is received. */
    void update_deadline();

    // For usage in std::map as a key.
    bool operator<(const Client &x) const { return player_name < x.player_name;}
};

// Game data structure.
struct State {
    game_id_t game_id;

    int turning_speed;

    dim_t width;
    dim_t height;
    std::vector<std::vector<bool>> pixels;

    std::vector<Event> history;
    std::unique_ptr<RandGenerator> gen;
    // Sorted by owners in alphabetical order.
    std::vector<worm_t> worms;

    State(int turning_speed, dim_t width, dim_t height, std::unique_ptr<RandGeneratr> gen) : turning_speed(turning_speed), width(width), height(height), pixels(height, std::vector<bool>(width, false)), gen(std::move(gen)) {}

    // New round.
    size_t increment();
    // New game.
    void refresh();
};

constexpr int NAMES_DEFAULT = -1;
constexpr int NAMES_READY = 1;

struct Server {
    int sockfd;

    std::map<SockAddr, Client> clients;
    // Different roles while waiting for a game to start and while running a game.
    // Never contains an empty string as a key.
    // Default uninitialized value is -1.
    std::unordered_map<std::string, int> names;
    // Queue of message objects for sending messages to clients.
    // (advanced msg queue)
    std::queue<Server2Client> q;
    State game;

    Server(int sockfd, int turning_speed, dim_t width, dim_t height, uint32_t seed) : sockfd(sockfd), State(turning_speed, width, height, std::make_unique(DeterministicRandGenerator(seed))) {}

    bool contains(const clientkey_t &) const;
    void add(const Sockaddr &, session_id_t, const std::string &);
    void remove(const Sockaddr &);
    void update(const Sockaddr &addr) { clients[addr].update(); }

    void add_msg(const clientkey_t &key, uint32_t ev_no) { q.push(Server2Client(key, ev_no)); }
    // Adds broadcast type message to the queue for events from specified point up to the end of the game history.
    void add_broadcast_msg(uint32_t);

    std::pair<int, received_t> receive(byte_t *, ssize_t &, size_t);
    void wait_receive(byte_t *, ssize_t &, size_t, size_t &);

    void send();
    bool Qempty() { return q.empty(); }
};
