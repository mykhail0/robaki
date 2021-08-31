#ifndef TO_GAME_H
#define TO_GAME_H

#include "../consts.h"
#include <string>

// Class for messages from the client to the game server.
class Client2Game {
private:
    byte_t bytes[MAX_2_GAME_MSG_LEN];
    size_t sz;
public:
    Client2Game(session_id_t, byte_t, uint32_t, const std::string &);
    // Functions for serializing.
    byte_t const *data() const { return bytes; };
    size_t size() const { return sz; }
};

#endif /* TO_GAME_H */
