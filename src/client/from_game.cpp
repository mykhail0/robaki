#include "from_game.h"
#include "../crc32.h"

namespace {
    // A wrapper for T BEbytes2num() in the context of
    // reading data from the specific buffer. `len` is the size of the buffer,
    // `offset` is it's current offset.
    // Returns true if success, false otherwise.
    // `offset` is advanced accordingly if the call was successful.
    template <class T>
    bool read_num(T &ret, byte_t *bytes, ssize_t len, size_t &offset, const std::string &err_msg = "") {
        size_t type_sz = sizeof(T);
        if ((size_t) len < offset + type_sz) {
            perror(err_msg.c_str());
            return -1;
        }
        ret = BEbytes2num<T>(bytes + offset);
        offset += type_sz;
        return 0;
    }

    // Just a call for read_num() with a specific message.
    template <class T>
    bool read_num_from_event(T &ret, byte_t *bytes, ssize_t len, size_t &offset) {
        return read_num(ret, bytes, len, offset, "Not enough bytes received to read the event message.");
    }

    // (sizeof event_no) + sizeof event_type
    constexpr size_t EVENT_LEN_WO_DATA = 5;
}

Game::Game(game_id_t gid, const NewGame &e) : game_id(gid), smallest_not_sent(0), ev0(e), sending_the_rest(false) {
    event_nums.insert(0);
    auto v = e.get_names();
    for (size_t i = 0; i < v.size(); ++i)
        names[i] = v[i];
}

void Game::add(uint32_t ev_no, std::unique_ptr<Serializable2GUI> e) {
    event_nums.insert(ev_no);
    received[ev_no] = std::move(e);
} 

bool Game::send(int sockfd) {
    auto l = expected();
    if (smallest_not_sent >= l)
        return true;

    if (smallest_not_sent == 0) {
        if (not ev0.send2GUI(sockfd))
            return false;
        ++smallest_not_sent;
    }

    for (std::map<uint32_t, std::unique_ptr<Serializable2GUI>>::iterator it = received.begin();
         it != received.end() && it->first < l; ++smallest_not_sent) {
        if (not it->second->send2GUI(sockfd))
            return false;
        it = received.erase(it);
    }

    return true;
}

bool Game::send_the_rest(int sockfd) {
    sending_the_rest = true;
    for (std::map<uint32_t, std::unique_ptr<Serializable2GUI>>::iterator it = received.begin();
         it != received.end(); ++smallest_not_sent) {
        if (not it->second->send2GUI(sockfd))
            return false;
        it = received.erase(it);
    }

    return true;
}

bool Game2GUI::event_is_known(byte_t ev) const {
    return known_event_types.find(ev) != known_event_types.end();
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

void Game2GUI::recv(int sockfd, const SockAddr &game_srvr) {
    ssize_t len;
    sockaddr_storage client_address;
    socklen_t rcv_len = sizeof client_address;
    if ((len = rcv_msg(sockfd, &client_address, &rcv_len, from_game, MAX_GAME_MSG_LEN)) <= 0)
        return;
    if (!(SockAddr(&client_address, rcv_len) == game_srvr)) {
        perror("Received message from someone different than server.");
        return;
    }

    size_t offset = 0;
    game_id_t game_id_srvr = 0;
    if (not read_num(game_id_srvr, from_game, len, offset, "Datagram smaller than game_id_t.\n"))
        return;

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

    while (offset < (size_t) len) {
        uint32_t event_len, event_no, checksum;
        byte_t event_type = 0;
        size_t old_offset, event_data_len;

        // Works becuase of lazy evaluation.
        if (not read_num_from_event(event_len, from_game, len, offset) ||
            event_len < EVENT_LEN_WO_DATA ||
            not read_num_from_event(event_no, from_game, len, offset) ||
            not read_num_from_event(event_type, from_game, len, offset) ||
            // This always evaluates to false. Reassigns old_offset, then event_data_len and then offset.
            (old_offset = offset, event_data_len = event_len - EVENT_LEN_WO_DATA, offset += event_data_len, false) ||
            not read_num_from_event(checksum, from_game, len, offset) ||
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
                q.back().add(event_no, std::make_unique<Pixel>(from_game + old_offset, event_data_len, q.back().names));
                break;
            case PLAYER_ELIMINATED:
                q.back().add(event_no, std::make_unique<PlayerEliminated>(from_game + old_offset, event_data_len, q.back().names));
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
