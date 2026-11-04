#ifndef SERVER_CONFIG_H
#define SERVER_CONFIG_H

#include <stdexcept>

#include "../consts.h"

namespace server {
// Class for checking command line arguments
// and extracting server parameters.
class config_t {
 private:
  uint16_t port_num;
  uint32_t seed;
  uint16_t rounds_per_sec;
  uint8_t turning_speed;
  dim_t width;
  dim_t height;

 public:
  config_t(int argc, char* argv[]);
  uint16_t get_port_num() const { return port_num; }
  uint32_t get_seed() const { return seed; }
  uint16_t get_rps() const { return rounds_per_sec; }
  uint8_t get_turning_speed() const { return turning_speed; }
  dim_t get_width() const { return width; }
  dim_t get_height() const { return height; }
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
