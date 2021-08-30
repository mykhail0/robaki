#include "from_game.h"

namespace {
    std::invalid_argument &invalid_length() {
        static std::invalid_argument e("Bad event length for the given type.");
        return e;
    }

    std::invalid_argument &invalid_player_num() {
        static std::invalid_argument e("Invalid player number.");
        return e;
    }

    template <class T>
    size_t num_size(T x) {
        return to_string(x).size();
    }
}

// "NEW_GAME [maxx] [maxy] ".size() is at least 11
NewGame::NewGame(byte_t *bytes, size_t len) : off(0), bytes(nullptr), sz(11) {
    size_t required = sizeof maxx + sizeof maxy;
    if (len < required || MAX_SRVR_NEWGAME_MSG_LEN < len)
        throw std::invalid_argument("Bad NEW_GAME event length.");

    size_t i = 0;

    maxx = BEbytes2num<dim_t>(bytes + i);
    i += sizeof maxx;

    maxy = BEbytes2num<dim_t>(bytes + i);
    i += sizeof maxy;

    std::string names_str((const char *) (bytes + i), len - required);
    if (names_str.back() != 0)
        throw std::invalid_argument("No '\\0' at the end of player_name list.");
    std::queue<int> null_ind;
    for (auto c: names_str) {
        if (c == 0)
            null_ind.push(i);
        ++i;
    }

    for (i = 0; not null_ind.empty(); null_ind.pop()) {
        player_name_list.push_back(std::string(names_str, null_ind.front() - i));
        i = null_ind.front() + 1;
        if (player_name.back().size() > 20)
            throw std::invalid_argument("Too long player_name.");
    }

    if (player_name_list.size() > MAX_GAMERS_NUM)
        throw std::invalid_argument("Too many players.");

    sz += num_size<dim_t>(maxx) + num_size<dim_t>(maxy);

    for (auto &s: player_name_list) {
        sz += s.size() + 1;
        for (auto c: s) {
            if (c < 33 || 126 < c)
                throw std::invalid_argument("Bad player name.");
        }
    }
}

byte_t *NewGame::data() {
    if (bytes != nullptr)
        return bytes + off;

    std::string str = "NEW_GAME " + to_string(maxx) + " " + to_string(maxy) + " ";
    for (size_t j = 0; j + 1 < player_name_list.size(); ++j)
        str += player_name_list[j] + " ";
    str += player_name_list.back() + "\n";

    bytes = new bytes[size()];
    memcpy(bytes, str.c_str(), str.size());
    return bytes + off;
}

// "PIXEL [x] [y] [name]\n".size() is at least 9
Pixel::Pixel(byte_t *bytes, size_t len, const std::unordered_map<byte_t, std::string> &names) : off(0), bytes(nullptr), sz(9) {
    if (len != SRVR_PIXEL_MSG_LEN)
        throw invalid_length();

    size_t i = 0;

    auto player_number = bytes[i];
    i += sizeof player_number;

    if (player_number > MAX_GAMERS_NUM)
        throw invalid_player_num();

    x = BEbytes2num<dim_t>(bytes + i);
    i += sizeof x;

    y = BEbytes2num<dim_t>(bytes + i);

    player = names[player_number];
    sz += num_size<dim_t>(x) + num_size<dim_t>(y) + player.size();
}

byte_t *Pixel::data() {
    if (bytes != nullptr)
        return bytes + off;

    std::string str = "PIXEL " + to_string(x) + " " + to_string(y) + " " + player + "\n";
    bytes = new bytes[size()];
    memcpy(bytes, str.c_str(), str.size());
    return bytes + off;
}

// "PLAYER_ELIMINATED [player_name]\n".size() is at least 19.
PlayerEliminated::PlayerEliminated(byte_t *bytes, size_t len, const std::unordered_map<byte_t, std::string> &names) : sz(19), off(0) {
    if (len != SRVR_PLAYERELIMINATED_MSG_LEN)
        throw invalid_length();

    auto player_number = bytes[0];
    if (player_number > MAX_GAMERS_NUM)
        throw invalid_player_num();

    player = names[player_number];
    sz += player.size();
}

byte_t *PlayerEliminated::data() {
    if (bytes != nullptr)
        return bytes + off;

    std::string str = "PLAYER_ELIMINATED  " + player + "\n";
    bytes = new bytes[size()];
    memcpy(bytes, str.c_str(), str.size());
    return bytes + off;
}

GameOver::GameOver(size_t len) {
    if (len != SRVR_GAMEOVER_MSG_LEN)
        throw invalid_length();
}

Game::Game(game_id_t gid, const NewGame &e) : game_id(gid), ev0(e), smallest_not_sent(0), sending_the_rest(false) {
    event_nums.insert(0);
    auto v = x.get_names();
    for (size_t i = 0; i < v.size(); ++i)
        names[i] = v[i];
}

void Game::add(uint32_t ev_no, const std::unique_ptr<Serializable2GUI> e) {
    event_nums.insert(ev_no);
    received[ev_no] = e;
} 

// Returns 0 if success, -1 otherwise.
// Wrapper for T BEbytes2num() in the context of the buffer size and its offset.
template <class T>
int read_num(T &ret, byte_t *bytes, ssize_t len, size_t &offset, const std::string &err_msg = "") {
    size_t type_sz = sizeof(T);
    if (len < offset + type_sz) {
        perror(err_msg);
        return -1;
    }
    ret = BEbytes2num<T>(bytes + offset);
    offset += type_sz;
    return 0;
}

// Just a call for read_num() with specific message.
template <class T>
int read_num_from_event(T &ret, byte_t *bytes, ssize_t len, size_t &offset) {
    return read_num(ret, bytes, len, offset, "Not enough bytes received to read the event message.");
}

void Game2GUI::recv(int sockfd, const Sockaddr &game_srvr) {
    ssize_t len;
    sockaddr_storage client_address;
    socklen_t rcv_len = sizeof client_address;
    if ((len = rcv_msg(sockfd, &client_address, &rcv_len, from_game, MAX_SRVR_MSG_LEN)) <= 0)
        return;
    if (!(server_addr(&client_address, rcv_len) == game_srvr)) {
        perror("Received message from someone different than server.");
        return;
    }

    size_t offset = 0;

    if (len < offset + sizeof(game_id_t)) {
        perror("Datagram smaller than game_id_t");
        return;
    }
    auto game_id_srvr = BEbytes32(from_game + offset);
    offset += sizeof(game_id_t);

    // 1) Ignored from now on, but previous events are ok:
    // * bad checksum
    // * received datagram is truncated
    // (not enough bytes received to read the event message)
    // * len field is smaller than EVENT_LEN_WO_DATA.
    // 2) exit(1) with error message
    // * good checksum, known event_type and nonsense data for the protocol
    // (strange player names or event length is not ok for this event_type)
    // 3) This event is ignored but previous and next events are ok:
    // * good checksum, but unknown event_type

    while (offset < len) {
        uint32_t event_len, event_no, checksum;
        byte_t event_type;
        size_t old_offset, event_data_len;

        // Works becuase of lazy evaluation.
        if (read_num_from_event<uint32_t>(event_len, from_game, len, offset) != 0 ||
            event_len < EVENT_LEN_WO_DATA ||
            read_num_from_event<uint32_t>(event_no, from_game, len, offset) != 0 ||
            read_num_from_event<byte_t>(event_type, from_game, len, offset) != 0 ||
            // This always evaluates to false. Reassigns old_offset, then event_data_len and then offset.
            (old_offset = offset, event_data_len = event_len - EVENT_LEN_WO_DATA, offset += event_data_len, false) ||
            read_num_from_event<uint32_t>(checksum, from_game, len, offset) != 0 ||
            crc32(from_game + old_offset, event_data_len) != checksum) {
            // case 1)
            return;
        }

        // event_data starts at old_offset
        // event_len, event_no, event_type and checksum are properly assigned
        // offset points at the start of the next event
        // checksum is good

        if (not event_is_known(event_type))
            continue;

        try {
            if (q.empty() || game_id_srvr != q.back().id()) {
                if (event_type != NEW_GAME)
                    continue;
            }

            switch (event_type) {
            case NEW_GAME:
                q.push(Game(game_id_srvr, NewGame(from_game + old_offset, event_data_len)));
                break;
            case PIXEL:
                q.back().add(event_no, std::make_unique<Pixel>(from_game + old_offset, event_data_len));
                break;
            case PLAYER_ELIMINATED:
                q.back().add(event_no, std::make_unique<PlayerEliminated>(from_game + old_offset, event_data_len));
                break;
            case GAME_OVER:
                GameOver e(event_data_len);
                break;
            }
        } catch (const std::exception &e) {
            // case 2)
            perror(e.what());
            exit(1);
        }
    }
}

bool Game::send_1msg(int sockfd, const std::unique_ptr<Serializable2GUI> &ev) {
    ssize_t sent = 0;
    while (not ev.finished()) {
        sent = send(sockfd, ev.data(), ev.size(), 0);
        ret += sent;
        auto err = errno;
        if (sent != -1) {
            ev.offset(sent);
        } else {
            if (err == EWOULDBLOCK || err == EAGAIN) {
                return false;
            } else {
                syserr("send_to_gui()");
            }
        }
    }
    return true;
}

bool Game::send_the_rest(int sockfd) {
    sending_the_rest = true;
    for (std::map<uint32_t, std::unique_ptr<Serializable2GUI>>::iterator it = received.begin();
         it != received.end(); ++smallest_not_sent) {
        if (send_1msg_to_gui(sockfd, it->second) == -1)
            return false;
        it = received.erase(it);
    }

    return true;
}

bool Game::send(int sockfd) {
    auto l = expected();
    if (smallest_not_sent >= l)
        return true;

    if (smallest_not_sent == 0) {
        if (send_1msg_to_gui(sockfd, ev0) == -1)
            return false;
        ++smallest_not_sent;
    }

    for (std::map<uint32_t, std::unique_ptr<Serializable2GUI>>::iterator it = received.begin();
         it != received.end() && it->first < l; ++smallest_not_sent) {
        if (send_1msg_to_gui(sockfd, it->second) == -1)
            return false;
        it = received.erase(it);
    }

    return true;
}

bool Game2GUI::send(int sockfd) {
    bool ret = true;
    while (not q.empty()) {
        if (not q.front().is_sending_the_rest())
            ret = q.front().send(sockfd);
        if (not ret)
            return ret;

        if (q.front().is_sending_the_rest() || q.size() > 1) {
            ret = q.front().send_the_rest(sockfd);
            if (not ret)
                return ret;
            q.pop();
        } else {
            return ret;
        }
    }
    return true;
}
