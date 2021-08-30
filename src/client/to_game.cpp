#include "to_game.h"

Client2Game::Client2Game(session_id_t session_id, byte_t turn_direction, uint32_t next_expected_event_no, const std::string &player_name) : sz(13) {
    session_id = htobe64(session_id);
    size_t i = 0, l = sizeof session_id;
    memcpy(bytes + i, &session_id, l);
    i += l;

    l = sizeof turn_direction;
    memcpy(bytes + i, &turn_direction, l);
    i += l;

    next_expected_event_no = htonl(next_expected_event_no);
    l = sizeof next_expected_event_no;
    memcpy(bytes + i, &next_expected_event_no, l);
    i += l;

    l = player_name.size();
    memcpy(bytes + i, player_name.c_str(), l);
    sz += l;
}
