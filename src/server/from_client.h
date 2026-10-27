#ifndef FROM_CLIENT_H
#define FROM_CLIENT_H

#include <string>

#include "../consts.h"

namespace server {
struct from_client_t {
  session_id_t session_id;
  byte_t turn_direction;
  uint32_t next_expected_event_no;
  std::string player_name;

  from_client_t(byte_t*, size_t);
  void print() const;
};
}  // namespace server

#endif /* FROM_CLIENT_H */
