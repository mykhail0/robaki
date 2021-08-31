#include "events.h"
#include "../err.h"
#include <cstring>
#include <queue>

namespace {
    // Below are a few common exceptions thrown while parsing event_data field
    // of messages from the game server.
    std::invalid_argument &invalid_length() {
        static std::invalid_argument e("Bad event length for the given type.");
        return e;
    }

    std::invalid_argument &invalid_player_num() {
        static std::invalid_argument e("Invalid player number.");
        return e;
    }

    // Returns the number of digits of a given number.
    template <class T>
    size_t num_size(T x) {
        return std::to_string(x).size();
    }
}

bool Serializable2GUI::send2GUI(int sockfd) {
    ssize_t sent = 0;
    while (not finished()) {
        sent = send(sockfd, data(), size(), 0);
        auto err = errno;
        if (sent != -1) {
            offset(sent);
        } else {
            if (err == EWOULDBLOCK || err == EAGAIN) {
                return false;
            } else {
                syserr("bool Serializable2GUI::send(int)");
            }
        }
    }
    return true;
}

// "NEW_GAME___".size() == 11
NewGame::NewGame(byte_t *bytes, size_t len) : Serializable2GUI(11) {
    size_t required = sizeof maxx + sizeof maxy;
    if (len < required || MAX_GAME_NEWGAME_MSG_LEN < len)
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
        if (player_name_list.back().size() > 20)
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

byte_t const *NewGame::data() const {
    if (bytes != nullptr)
        return bytes + off;

    std::string str = "NEW_GAME " +
        std::to_string(maxx) + " " + std::to_string(maxy) + " ";
    for (size_t j = 0; j + 1 < player_name_list.size(); ++j)
        str += player_name_list[j] + " ";
    str += player_name_list.back() + "\n";

    bytes = new byte_t[size()];
    memcpy(bytes, str.c_str(), str.size());
    return bytes + off;
}

// "PIXEL____".size() == 9
Pixel::Pixel(byte_t *bytes, size_t len,
    const std::unordered_map<byte_t, std::string> &names) : Serializable2GUI(9) {
    if (len != GAME_PIXEL_MSG_LEN)
        throw invalid_length();

    size_t i = 0;

    auto player_number = bytes[i];
    i += sizeof player_number;

    if (player_number > MAX_GAMERS_NUM)
        throw invalid_player_num();

    x = BEbytes2num<dim_t>(bytes + i);
    i += sizeof x;

    y = BEbytes2num<dim_t>(bytes + i);

    player = names.at(player_number);
    sz += num_size<dim_t>(x) + num_size<dim_t>(y) + player.size();
}

byte_t const *Pixel::data() const {
    if (bytes != nullptr)
        return bytes + off;

    std::string str = "PIXEL " + std::to_string(x) + " " + std::to_string(y) + " " + player + "\n";
    bytes = new byte_t[size()];
    memcpy(bytes, str.c_str(), str.size());
    return bytes + off;
}

// "PLAYER_ELIMINATED__".size() == 19
PlayerEliminated::PlayerEliminated(byte_t *bytes, size_t len,
    const std::unordered_map<byte_t, std::string> &names) : Serializable2GUI(19) {
    if (len != GAME_PLAYERELIMINATED_MSG_LEN)
        throw invalid_length();

    auto player_number = bytes[0];
    if (player_number > MAX_GAMERS_NUM)
        throw invalid_player_num();

    player = names.at(player_number);
    sz += player.size();
}

byte_t const *PlayerEliminated::data() const {
    if (bytes != nullptr)
        return bytes + off;

    std::string str = "PLAYER_ELIMINATED  " + player + "\n";
    bytes = new byte_t[size()];
    memcpy(bytes, str.c_str(), str.size());
    return bytes + off;
}

GameOver::GameOver(size_t len) {
    if (len != GAME_GAMEOVER_MSG_LEN)
        throw invalid_length();
}
