#ifndef TO_GAME_H
#define TO_GAME_H

#include <array>
#include <string>

#include "../consts.h"
#include "../utility.h"

// Class for messages from the client to the game server.
class ClientToGame : public Serializable {
 private:
  std::array<byte_t, MAX_2_GAME_MSG_LEN> bytes;
  size_t sz;

 public:
  ClientToGame(session_id_t, byte_t, uint32_t, const std::string&);
  // Serializable interface implementation.
  byte_t const* data() const { return bytes.data(); };
  size_t size() const { return sz; }
};

#endif /* TO_GAME_H */
