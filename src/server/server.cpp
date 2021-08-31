#include "server.h"
#include "from_client.h"
#include <cassert>
#include <cstring>
#include <algorithm>

Server2Client::Server2Client(
    const std::stack<clientkey_t> &recipients, ssize_t start, ssize_t end
) : recipients(recipients), ev_no(start), start(ev_no), end(end) {}

Server2Client::Server2Client(const clientkey_t &recipient, ssize_t ev_no) :
    ev_no(ev_no), start(ev_no), end(-1) { recipients.push(recipient); }

int Server2Client::send(const Server &s) {
    // Alias.
    auto &hist = s.game.history;

    ssize_t g = end == -1 ? hist.size() : end;
    bool tmp = hist.size() >= (size_t) g;
    if (not tmp)
        perror("New game already started, but need to send messages for a previous one! (Queue was not emptied of redundant messages)");
    assert(tmp);

    while (not recipients.empty() && not s.contains(recipients.top()))
        recipients.pop();

    if (recipients.empty())
        return 0;

    byte_t datagram[MAX_GAME_MSG_LEN];
    auto id = htonl(s.game.game_id);
    uint32_t len = sizeof id;

    auto prev_ev_no = ev_no;
    while (ev_no < g && hist[ev_no].size() + len <= MAX_GAME_MSG_LEN) {
        memcpy(datagram + len, hist[ev_no].data(), hist[ev_no].size());
        len += hist[ev_no++].size();
    }

    memcpy(datagram, &id, sizeof id);

    auto ret = sendto(s.sockfd, datagram, len, 0,
        (sockaddr *) recipients.top().first.get_addr(),
        recipients.top().first.size());

    if (ret == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            ev_no = prev_ev_no;
        } else {
            perror("Error sening Server2Client\n");
        }
    }

    if (ev_no >= g) {
        recipients.pop();
        ev_no = start;
    }

    return ret;
}

Client::Client(session_id_t id, const std::string &player_name) : id(id),
    deadline(std::chrono::system_clock::now() + std::chrono::milliseconds {2000}),
    player_name(player_name) {}

void Client::update_deadline() {
    deadline = std::chrono::system_clock::now() + std::chrono::milliseconds {2000};
}

Server::Server(int sockfd, int turning_speed, dim_t width, dim_t height, uint32_t seed,
    int rounds_per_sec) : sockfd(sockfd), rounds_per_sec(rounds_per_sec),
    game(State(turning_speed, width, height,
        std::make_unique<DeterministicRandGenerator>(seed)
    )) {}

bool Server::contains(const clientkey_t &key) const {
    auto it = clients.find(key.first);
    return it != clients.end() && it->second.id == key.second;
}

void Server::add(const SockAddr &addr, session_id_t id, const std::string &player_name) {
    clients.insert({addr, Client(id, player_name)});
    if (not player_name.empty())
        names[player_name] = NAMES_DEFAULT;
}

void Server::remove(const SockAddr &addr) {
    auto it = clients.find(addr);
    names.erase(it->second.player_name);
    clients.erase(it);
}

void Server::update(const SockAddr &addr) {
    auto it = clients.find(addr);
    if (it != clients.end())
        it->second.update_deadline();
}

void Server::add_msg(const clientkey_t &key, uint32_t ev_no) {
    q.push(Server2Client(key, ev_no));
}

void Server::add_broadcast_msg(uint32_t start) {
    size_t end = game.history.size();
    std::stack<clientkey_t> recipients;
    for (auto const &it: clients)
        recipients.push({it.first, it.second.id});
    q.push(Server2Client(recipients, start, end));
}

void Server::send() {
    if (Qempty())
        return;
    q.front().send(*this);
    if (q.front().done())
        q.pop();
}

namespace {
    // Bits for the bitmask returned as part of the return type of Server::receive().
    // If REMOVED then also should be ADDED.
    constexpr int IGNORED = 1, REMOVED = 2, ADDED = 4;
}

std::pair<int, received_t> Server::receive() {
    received_t ret;
    int stat = IGNORED;
    ssize_t len;

    sockaddr_storage client_address;
    socklen_t rcv_len = sizeof client_address;
    if ((len = rcv_msg(sockfd, &client_address, &rcv_len, buffer, sizeof buffer)) <= 0)
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
            bool same_id_and_diff_name = old_session_id == msg.session_id &&
                msg.player_name != client_it->second.player_name;

            must_add = old_session_id < msg.session_id && not recognises_name;
            ignore = old_session_id > msg.session_id || same_id_and_diff_name ||
                (old_session_id < msg.session_id && recognises_name);

            if (must_add || same_id_and_diff_name) {
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

void Server::wait_receive(size_t &ready) {
    auto [stat, ret] = receive();
    if (stat == IGNORED)
        return;
    if (stat & REMOVED)
        ready -= ret.removed_val == 1;
    if (stat & ADDED) {
        auto it = names.find(ret.player_name);
        ready += it == names.end() ? 0 : it->second == 1;
    }
}

void Server::run_receive() {
    auto [stat, ret] = receive();
    if (stat & (IGNORED | ADDED))
        return;
    auto it = names.find(ret.player_name);
    if (it == names.end() || it->second == NAMES_DEFAULT)
        return;
    game.update_worm(it->second, ret.turn_direction);
}

void Server::check_for_idle_clients(size_t &ready) {
   for (auto it = clients.cbegin(); it != clients.cend(); ) {
        if (std::chrono::system_clock::now() > it->second.deadline) {
            auto it2 = names.find(it->second.player_name);
            if (it2 != names.end()) {
                ready -= it2->second == NAMES_READY;
                names.erase(it2);
            }
            it = clients.erase(it);
        } else {
            ++it;
        }
    }
}

void Server::check_for_idle_clients() {
   for (auto it = clients.cbegin(); it != clients.cend(); ) {
        if (std::chrono::system_clock::now() > it->second.deadline) {
            auto it2 = names.find(it->second.player_name);
            if (it2 != names.end())
                names.erase(it2);
            it = clients.erase(it);
        } else {
            ++it;
        }
    }
}

void Server::game_ready() {
    std::vector<std::string> v;
    for (auto const &it: names) {
        if (it.first.empty())
            continue;
        v.push_back(it.first);
    }
    std::sort(v.begin(), v.end());
    std::string player_name_list;
    size_t i = 0;
    for (auto &s: v) {
        player_name_list += s + std::string(1, (char) 0);
        names[s] = i++;
    }
    add_broadcast_msg(game.refresh(
        Event(NewGame(game.width, game.height, player_name_list.size(), player_name_list.c_str())),
        v.size())
    );
}

void Server::game_over() {
    for (auto &it: names)
        it.second = NAMES_DEFAULT;
}
