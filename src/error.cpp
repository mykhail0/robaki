#include "error.h"

std::string critical_error::format_message(kind ec, const std::string& detail) {
  std::string msg;
  switch (ec) {
    case kind::failed_send:
      msg = "Failed send() call";
      break;
    case kind::failed_recv:
      msg = "Failed recv() call";
      break;
    case kind::failed_timer:
      msg = "Failed to setup a timer";
      break;
    default:
      msg = "Unknown critical error";
      break;
  }
  if (not detail.empty()) {
    msg.append(": ").append(detail);
  }
  return msg;
}

critical_error::critical_error(kind ec, const std::string& detail)
    : std::runtime_error(format_message(ec, detail)), error_kind_(ec) {}
