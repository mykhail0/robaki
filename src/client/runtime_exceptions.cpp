#include "runtime_exceptions.h"

std::string client::parsing_error::format_message(kind ec,
                                                  const std::string& detail) {
  std::string msg;
  switch (ec) {
    case kind::invalid_length:
      msg = "Bad event length for the given type";
      break;
    case kind::invalid_player_number:
      msg = "Invalid player number";
      break;
    case kind::invalid_format:
      msg = "Message is not in a valid format";
      break;
    default:
      msg = "Unknown parsing error";
      break;
  }
  if (not detail.empty()) {
    msg.append(": ").append(detail);
  }
  return msg;
}

client::parsing_error::parsing_error(kind ec, const std::string& detail)
    : std::runtime_error(format_message(ec, detail)), error_kind_(ec) {}
