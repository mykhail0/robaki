#include "from_client.h"
#include "../utility.h"

#include <iostream>

Client2Server::Client2Server(byte_t *bytes, size_t len) {
    size_t required = sizeof session_id + sizeof turn_direction + sizeof next_expected_event_no;
    if (len < required || MAX_2_GAME_MSG_LEN < len)
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

void Client2Server::print() const {
    std::cerr << "Received message from client: "
        << "session id: " << session_id << " turn direction: " << turn_direction
        << " next expected event no: " << next_expected_event_no
        << " player name: " << player_name << "\n";
}
