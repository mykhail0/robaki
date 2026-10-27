#include "config.h"

#include <unistd.h>

#include <stdexcept>

#include "../utility.h"

client::client_parameter_error::client_parameter_error(
    kind ec, const std::string& detail)
    : std::runtime_error(format_message(ec, detail)), error_kind_(ec) {}

std::string client::client_parameter_error::format_message(
    kind ec, const std::string& detail) {
  static const std::string usage =
      "\nUsage: ./screen-worms-client game_server [-n player_name] [-p n] [-i "
      "gui_server] [-r n]\n  * game_server – adress (IPv4 or IPv6) or game "
      "server name\n  * -n player_name – player name, 0-20 ASCII characters in "
      "range of [33, 126]\n  * -p n – game server port (2021 by default)\n  "
      "* -i gui_server – adress (IPv4 or IPv6) or gui server name (localhost by"
      " default)\n  * -r n – gui server port (20210 by default)\n";
  std::string msg;
  switch (ec) {
    case kind::too_few_arguments:
      msg = "Too few arguments, expected game server name";
      break;
    case kind::player_name_too_long:
      msg = "Player name is longer than 20 characters";
      break;
    case kind::invalid_characters:
      msg = "A character in player name out of [33, 126] ASCII range";
      break;
    case kind::nonoption_argument:
      msg = "Nonoption arguments not allowed";
      break;
    case kind::unknown_parameter:
      msg = "Unknown parameter";
      break;
    case kind::invalid_port_number:
      msg = "Invalid port number";
      break;
    default:
      msg = "Unknown client parameter error";
      break;
  }
  if (not detail.empty()) {
    msg.append(": ").append(detail);
  }
  msg.append(usage);
  return msg;
}

client::config_t::config_t(int argc, char* argv[])
    :  // Default configuration.
      port_num(2021),
      gui("localhost"),
      gui_port(20210) {
  if (argc < 2) {
    throw client_parameter_error(
        client_parameter_error::kind::too_few_arguments);
  }
  game_server = argv[1];

  int opt;
  while ((opt = getopt(argc - 1, argv + 1, "n:p:i:r:")) != -1) {
    switch (opt) {
      case 'n':
        player_name = optarg;
        if (player_name.size() > MAX_PLAYER_NAME_LEN) {
          throw client_parameter_error(
              client_parameter_error::kind::player_name_too_long);
        }
        for (char c : player_name) {
          if (c < 33 or 126 < c) {
            throw client_parameter_error(
                client_parameter_error::kind::invalid_characters,
                std::string("encountered character ").append(1, c));
          }
        }
        break;
      case 'p':
        try {
          long t = stol_wrap(optarg);
          check_port(t);
          port_num = t;
        } catch (const std::invalid_argument& e) {
          throw client_parameter_error(
              client_parameter_error::kind::invalid_port_number, e.what());
        } catch (const std::out_of_range& e) {
          throw client_parameter_error(
              client_parameter_error::kind::invalid_port_number, e.what());
        }
        break;
      case 'i':
        gui = optarg;
        break;
      case 'r':
        try {
          long t = stol_wrap(optarg);
          check_port(t);
          gui_port = t;
        } catch (const std::invalid_argument& e) {
          throw client_parameter_error(
              client_parameter_error::kind::invalid_port_number, e.what());
        } catch (const std::out_of_range& e) {
          throw client_parameter_error(
              client_parameter_error::kind::invalid_port_number, e.what());
        }
        break;
      default: /* '?' */
        throw client_parameter_error(
            client_parameter_error::kind::unknown_parameter);
    }
  }

  if (optind < argc - 1) {
    throw client_parameter_error(
        client_parameter_error::kind::nonoption_argument);
  }
}
