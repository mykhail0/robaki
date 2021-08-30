// FIXME after round ends send shit from queue until cant no more

/*

#include <arpa/inet.h>
#include <endian.h>
#include <cstring>
#include "crc32.h"
#include <unordered_map>

#include <cstdio>
#include <cstdint>
#include <cstring>

#include <signal.h>

#include "config.h"
#include "server.h"
#include "err.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <poll.h>

#include <algorithm>
#include <map>
#include <set>
#include <unordered_set>
*/

/*
#include <sys/timerfd.h>
*/

// static constants.
uint32_t Event::cnt = 0;
long double play_data_t::PI = 3.14159265358979323846;

NewGame::NewGame(
    dim_t maxx, dim_t maxy, size_t list_len, char *player_name_list) : list_len(list_len) {
    byte_t *start = bytes;

    auto tmp = htonl(maxx);
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    tmp = htonl(maxy);
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    memcpy(start, player_name_list, list_len);
}

Pixel::Pixel(byte_t player_number, dim_t x, dim_t y) {
    byte_t *start = bytes;

    memcpy(start, &player_number, sizeof player_number);
    start += sizeof player_number;

    auto tmp = htonl(x);
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    tmp = htonl(y);
    memcpy(start, &tmp, sizeof tmp);
}

Event::Event(EventData &event_data) : event_no(Event::cnt++),
    event_type(event_data.event_type()) {

    size_t data_len = event_data.size();
    len = sizeof event_no + sizeof event_type + data_len;

    byte_t *start = bytes;

    auto tmp = htonl(len);
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    tmp = htonl(event_no);
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    memcpy(start, &event_type, sizeof event_type);
    start += sizeof event_type;

    memcpy(start, event_data.data(), data_len);
    start += data_len;

    tmp = htonl(crc32(bytes, sizeof len + len));
    memcpy(start, &tmp, sizeof tmp);
    start += sizeof tmp;

    datagram_sz = start - bytes;
}

int Server2Client::send(const Server &s) {
    // Alias
    auto &hist = s.game.history;

    assert(("New game already started, but need to send messages for a previous one! (Queue was not emptied of redundant messages)", hist.size() < end));

    while (not recipients.empty() && not s.contains(recipients.top()))
        recipients.pop();

    if (recipients.empty())
        return 0;

    byte_t datagram[MAX_SRVR_MSG_LEN];
    auto id = htonl(s.game.game_id);
    event_len_t len = sizeof id;

    size_t g = end == -1 ? hist.size() : end;
    while (ev_no < g && hist[ev_no].size() + len <= MAX_SRVR_MSG_LEN) {
        memcpy(datagram + len, hist[ev_no].data(), hist[ev_no].size());
        len += hist[ev_no++].size();
    }

    memcpy(datagram, &id, sizeof id);

    auto ret = sendto(s.sockfd, datagram, len, 0,
        (sockaddr *) recipients.top().first.get_addr(),
        recipients.top().first.size());

    if (ev_no >= g) {
        recipients.pop();
        ev_no = start;
    }

    return ret;
}

void worm_t::refresh(uint32_t randX, uint32_t randY, uint32_t randDir, dim_t width, dim_t height) {
    last_turn = 0;
    x = randX % (width - 1) + (long double) 0.5L;
    y = randY % (height - 1) + (long double) 0.5L;
    direction = randDir % 360;
}

bool worm_t::increment(int turning_speed) {
    if (last_turn != 0) {
        if (last_turn == 1) {
            direction += turning_speed;
            direction %= 360;
        } else {
            direction += direction < turning_speed ? 360 : 0;
            direction -= turning_speed;
        }
    }
    long double angle = worm_t::PI * (360 - direction) / 180;
    dx = cos(angle);
    dy = - sin(angle);
    x += dx;
    y += dy;
    return ((int32_t) x == (int32_t) (x - dx)) &&
        ((int32_t) y == (int32_t) (y - dy));
}

void Client::update_deadline() {
    deadline = std::chrono::system_clock::now() + std::chrono::milliseconds {2000};
}

bool Server::contains(const clientkey_t &key) const {
    auto it = clients.find(key.first);
    return it != clients.end() && it->second.first = key.second;
}

void Server::add(const Sockaddr &addr, session_id_t id, const std::string &player_name) {
    clients[addr] = Client(id, player_name);
    if (not player_name.empty())
        names[player_name] = NAMES_DEFAULT;
}

void Server::remove(const Sockaddr &addr) {
    auto it = clients.find(addr);
    names.erase(it->second.second.player_name);
    clients.erase(it);
}

void Server::add_broadcast_msg(uint32_t start) {
    size_t end = game.history.size();
    std::stack<clientkey_t> recipients;
    for (auto &it: clients)
        recipients.push({it->first, it->second.id});
    q.push(Server2Client(recipients, start, end));
}

void Server::send() {
    if (Qempty())
        return;
    q.front().send(*this);
    if (q.front().done())
        q.pop();
}

Client2Server::Client2Server(byte_t *bytes, size_t len) {
    size_t required = sizeof session_id + sizeof turn_direction + sizeof next_expected_event_no;
    if (len < required || MAX_2_SRVR_MSG_LEN < len)
        throw std::invalid_argument("bad client datagram length");

    size_t i = 0;

    session_id = BEbytes2num<session_id_t>(bytes + i);
    i += sizeof session_id;

    turn_direction = bytes[i];
    i += sizeof turn_direction;

    next_expected_event_no = BEbytes2num<uint32_t>(bytes + i);
    i += sizeof next_expected_event_no;

    player_name = std::string((const char *) (bytes + i), len - required);
    for (auto c : player_name) {
        if (c < 33 || 126 < c)
            throw std::invalid_argument("bad player name");
    }
}

struct received_t {
    byte_t turn_direction;
    std::string player_name;
    std::string removed;
    int removed_val;

    received_t() : turn_direction(0), removed_val(-1) {}
};

constexpr int IGNORED = 1;
constexpr int REMOVED = 2;
constexpr int ADDED = 4;

std::pair<int, received_t> Server::receive(byte_t *buffer, ssize_t &len, size_t buflen) {
    received_t ret:
    int stat = IGNORED;

    sockaddr_storage client_address;
    socklen_t rcv_len = sizeof client_address;
    if ((len = rcv_msg(sockfd, &client_address, &rcv_len, buffer, buflen)) <= 0)
        return {stat, ret};

    try {
        SockAddr sender(&client_address, rcv_len);
        // Decipher the message.
        Client2Server msg(buffer, len);

        auto client_it = clients.find(sender);
        auto name_it = names.find(msg.player_name);

        bool recognises_addr = client_it != clients.end(),
            recognises_name = name_it != names.end();

        bool must_add = not recognises_addr && not recognises_name,
            ignore = not recognises_addr && recognises_name;

        if (recognises_addr) {
            assert(must_add == ignore);
            assert(ignore == false);

            auto old_session_id = client_it->second.id;
            bool same_id_&_diff_name = old_session_id == msg.session_id &&
                msg.player_name != client_it->second.player_name;

            must_add = old_session_id < msg.session_id && not recognises_name;
            ignore = old_session_id > msg.session_id || same_id_&_diff_name ||
                (old_session_id < msg.session_id && recognises_name);

            if (must_add || same_id_&_diff_name) {
                ret.removed = client_it->second.player_name;
                auto tmp = names.find(ret.removed);
                ret.removed_val = tmp == names.end() ? -1 : tmp->second;
                remove(sender);
                stat |= REMOVED;
            }
        }

        if (ignore)
            // (Recognises the address, same session_id, different names) -> don't do anything after removing this client.
            // (Recognises the address, bigger (newer) session_id, recognises the name) -> don't do anything after removing this client.
            // (Recognises the address, smaller session_id)
            // (Doesn't recognise the address, recognises the name)
            return {stat, ret};

        stat &= (~IGNORED);

        if (must_add) {
            add(sender, msg.session_id, msg.player_name);
            if (not msg.player_name.empty() && msg.turn_direction != STRAIGHT)
                names[msg.player_name] = 1;
            stat |= ADDED;
        }

        add_msg({sender, msg.session_id}, msg.next_expected_event_no);
        ret.turn_direction = msg.turn_direction;
        ret.player_name = msg.player_name;

    } catch (const std::exception &e) {
        perror(e.what());
    }

    return {stat, ret};
}

void Server::wait_receive(byte_t *buffer, ssize_t &len, size_t buflen, size_t &ready) {
    auto [stat, ret] = receive(buffer, len, buflen);
    if (stat == IGNORED)
        return;
    if (stat & REMOVED)
        ready -= ret.removed_val == 1;
    if (stat & ADDED) {
        auto it = names.find(ret.player_name);
        ready += it == names.end() ? 0 : it->second == 1;
    }
}
