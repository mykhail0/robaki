#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <cstdint>
#include <stdexcept>
#include <string>

namespace client {
// Class for checking command line arguments and extracting client parameters.
struct config_t {
  std::string game_server;
  std::string player_name;
  uint16_t port_num;
  std::string gui;
  uint16_t gui_port;

  config_t(int, char* argv[]);
};

class client_parameter_error : public std::runtime_error {
 public:
  enum class kind {
    too_few_arguments,
    player_name_too_long,
    invalid_characters,
    unknown_parameter,
    nonoption_argument,
    invalid_port_number
  };

 private:
  kind error_kind_;
  static std::string format_message(kind ec, const std::string& detail);

 public:
  explicit client_parameter_error(kind ec, const std::string& detail = "");
};
}  // namespace client

#endif /* CLIENT_CONFIG_H */
