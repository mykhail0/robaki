#ifndef FROM_CLIENT_H
#define FROM_CLIENT_H

#include "../consts.h"
#include <string>

struct Client2Server {
    session_id_t session_id;
    byte_t turn_direction;
    uint32_t next_expected_event_no;
    std::string player_name;

    Client2Server(byte_t *, size_t);
};

#endif /* FROM_CLIENT_H */
