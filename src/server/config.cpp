#include "config.h"

#include <unistd.h>

#include <ctime>
#include <stdexcept>

#include "../utility.h"

server::server_parameter_error::server_parameter_error(
    kind ec, const std::string& detail)
    : std::runtime_error(format_message(ec, detail)), error_kind_(ec) {}

std::string server::server_parameter_error::format_message(
    kind ec, const std::string& detail) {
  static const std::string usage =
      "Usage: ./screen-worms-server [-p n] [-s n] [-t n] [-v n] [-w n] "
      "[-h n]\n  * `-p n` – port number (`2021` by default)\n  * `-s "
      "n` – random number generator's seed (time(NULL) by default)\n  "
      "* `-t n` – integer which determines turning speed (6 by "
      "default)\n  * `-v n` – integer which determines game's pace "
      "(rounds per second, 50 by default)\n  * `-w n` – map's width in "
      "pixels (640 by default)\n  * `-h n` – map's height in pixels "
      "(480 by default)\n";
  std::string msg;
  switch (ec) {
    case kind::nonoption_argument:
      msg = "Nonoption arguments not allowed";
      break;
    case kind::unknown_parameter:
      msg = "Unknown parameter";
      break;
    case kind::invalid_numeral_argument:
      msg = "Invalid numeral argument";
      break;
    default:
      msg = "Unknown server parameter error";
      break;
  }
  if (not detail.empty()) {
    msg.append(": ").append(detail);
  }
  msg.append(usage);
  return msg;
}

server::config_t::config_t(int argc, char* argv[])
    :  // Default configuration.
      port_num(2021),
      rounds_per_sec(50),
      turning_speed(6),
      width(640),
      height(480) {
  // Lower 32 bits.
  time_t cur = time(nullptr);
  seed = cur - ((cur >> 32) << 32);

  int opt;
  while ((opt = getopt(argc, argv, "p:s:t:v:w:h:")) != -1) {
    switch (opt) {
      case 'p':
        try {
          long t = stol_wrap(optarg);
          check_port(t);
          port_num = t;
        } catch (const std::invalid_argument& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        } catch (const std::out_of_range& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        }
        break;
      case 's':
        try {
          long t = stol_wrap(optarg);
          check_num(t, 0, UINT32_MAX, "Negative seed.", "Too large seed.");
          seed = t;
        } catch (const std::invalid_argument& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        } catch (const std::out_of_range& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        }
        break;
      case 't':
        try {
          long t = stol_wrap(optarg);
          check_num(t, 1, 90, "Nonpositive turning speed.",
                    "Too large turning speed.");
          turning_speed = t;
        } catch (const std::invalid_argument& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        } catch (const std::out_of_range& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        }
        break;
      case 'v':
        try {
          long t = stol_wrap(optarg);
          check_num(t, 1, 250, "Nonpositive number of rounds per second.",
                    "Too large number of rounds per second.");
          rounds_per_sec = t;
        } catch (const std::invalid_argument& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        } catch (const std::out_of_range& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        }
        break;
      // 720p restrictions.
      case 'w':
        try {
          long t = stol_wrap(optarg);
          check_num(t, 16, 2048, "Too small width.", "Too large width.");
          width = t;
        } catch (const std::invalid_argument& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        } catch (const std::out_of_range& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        }
        break;
      case 'h':
        try {
          long t = stol_wrap(optarg);
          check_num(t, 16, 2048, "Too small height.", "Too large height.");
          height = t;
        } catch (const std::invalid_argument& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        } catch (const std::out_of_range& e) {
          throw server_parameter_error(
              server_parameter_error::kind::invalid_numeral_argument, e.what());
        }
        break;
      default: /* '?' */
        throw server_parameter_error(
            server_parameter_error::kind::unknown_parameter);
    }
  }

  if (optind < argc) {
    throw server_parameter_error(
        server_parameter_error::kind::nonoption_argument);
  }
}
