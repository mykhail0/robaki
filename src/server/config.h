#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <stdexcept>

#include "../consts.h"

namespace server {
// Class for checking command line arguments
// and extracting server parameters.
struct config_t {
  uint16_t port_num;
  uint32_t seed;
  int rounds_per_sec;
  int turning_speed;
  dim_t width;
  dim_t height;

  config_t(int, char* argv[]);
};

class server_parameter_error : public std::runtime_error {
 public:
  enum class kind {
    unknown_parameter,
    nonoption_argument,
    invalid_numeral_argument
  };

 private:
  kind error_kind_;
  static std::string format_message(kind ec, const std::string& detail);

 public:
  explicit server_parameter_error(kind ec, const std::string& detail = "");
};
}  // namespace server

#endif /* SERVER_CONFIG_H */
