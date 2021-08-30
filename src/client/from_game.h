#ifndef FROM_GAME_H
#define FROM_GAME_H

// Maximum message length is:
// NEW_GAME maxx maxy player_name1 player_name2 …
// (8 + 1) + 2 * ((max length of maxx = 10) + 1) + 25 * (20 + 1) = 556
constexpr int MAX_2_GUI_MSG_LEN = 556;
// sizeof event_no + sizeof event_type
constexpr int EVENT_LEN_WO_DATA = 5;

class Serializable2GUI : public Serializable {
protected:
    size_t off;
    byte_t *bytes;
    size_t sz;

public:
    void offset(size_t x) { off += x; }
    bool finished() const { return off == sz; }
    size_t size() const { return sz; }
};

class NewGame : public Serializable2GUI {
    private:
    dim_t maxx;
    dim_t maxy;
    std::vector<std::string> player_name_list;

    public:
    NewGame(byte_t *, size_t);

    // Serializable 2GUI interface implementation;
    byte_t *data();

    ~NewGame() { delete bytes; }

    std::vector<std::string> get_names() { return player_name_list; }
};

class Pixel : public Serializable2GUI {
    private:
    dim_t x;
    dim_t y;
    std::string player;

    public:
    Pixel(byte_t *, size_t, const std::unordered_map<byte_t, std::string> &);

    // Serializable2GUI interface implementation.
    byte_t *data();

    ~Pixel() { delete bytes; }
};

class PlayerEliminated : public Serializable2GUI {
    private:
    std::string player;

    public:
    PlayerEliminated(byte_t *, size_t, const std::unordered_map<byte_t, std::string> &);

    // Serializable2GUI interface implementation.
    byte_t *data();

    ~PlayerEliminated() { delete bytes; }
};

class GameOver {
    public:
    GameOver(size_t);
};

class Game {
    game_id_t game_id;

    iset event_nums;
    uint32_t smallest_not_sent;
    std::map<uint32_t, std::unique_ptr<Serializable2GUI>> received;

    NewGame ev0;
    std::unordered_map<byte_t, std::string> names;

    bool sending_the_rest;

public:
    Game(game_id_t, const NewGame &);

    game_id_t id() const { return game_id; }
    void add(uint32_t, const std::unique_ptr<Serializable2GUI> &);
    void game_over(uint32_t ev_no);

    uint32_t expected const { return event_nums.smallest_not_included(); }

    int send_1msg(int, const std::unique_ptr<Serializable2GUI> &);
    int send(int);
    int send_the_rest(int);
    bool is_sending_the_rest() const { return sending_the_rest; }
};

class Game2GUI {
    private:
    std::queue<Game> q;
    std::unordered_set<byte_t> known_event_types;
    byte_t from_game[MAX_SRVR_MSG_LEN];

    public:
    Game2GUI() : known_event_types({NEW_GAME, PIXEL, PLAYER_ELIMINATED, GAME_OVER}) {}

    bool event_is_known(byte_t ev) const { known_event_types.find(ev) != known_event_types.end(); }

    void recv(int, const SockAddr &);
    bool send(int);
};

#endif /* FROM_GAME_H */
