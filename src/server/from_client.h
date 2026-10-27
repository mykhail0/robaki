#ifndef FROM_CLIENT_H
#define FROM_CLIENT_H

#include <stdexcept>
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

class parsing_error : public std::runtime_error {
 public:
  enum class kind { invalid_length, invalid_player_name, bad_turn_direction };

 private:
  kind error_kind_;
  static std::string format_message(kind ec, const std::string& detail);

 public:
  explicit parsing_error(kind ec, const std::string& detail = "");
};

}  // namespace server

#endif /* FROM_CLIENT_H */
