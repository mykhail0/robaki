#ifndef TO_GAME_H
#define TO_GAME_H

#include <array>
#include <string>

#include "../consts.h"
#include "../utility.h"

namespace client {
// Class for messages from the client to the game server.
class to_game_t : public serializable {
 private:
  std::array<byte_t, MAX_CLIENT_GAME_MSG_LEN> bytes;
  size_t sz;

 public:
  to_game_t(session_id_t, byte_t, uint32_t, const std::string&);
  // `serializable` interface implementation.
  byte_t const* data() { return bytes.data(); };
  size_t size() const { return sz; }
};
}  // namespace client

#endif /* TO_GAME_H */
