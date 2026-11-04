#ifndef CLIENT_RUNTIME_EXCEPTIONS_H
#define CLIENT_RUNTIME_EXCEPTIONS_H

#include <stdexcept>

namespace client {
class parsing_error : public std::runtime_error {
 public:
  enum class kind { invalid_length, invalid_player_number, invalid_format };

 private:
  kind error_kind_;
  static std::string format_message(kind ec, const std::string& detail);

 public:
  explicit parsing_error(kind ec, const std::string& detail = "");
};
}  // namespace client

#endif  // CLIENT_RUNTIME_EXCEPTIONS_H
