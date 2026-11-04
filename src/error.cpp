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
    case kind::timer_pollerr:
      msg = "Timer POLLERR in poll()";
      break;
    case kind::unexpected_poll_timeout:
      msg = "Unexpected poll() timeout";
      break;
    case kind::poll_error:
      msg = "Failed poll()";
      break;
    case kind::socket_pollerr:
      msg = "Socket POLLERR in poll()";
      break;
    case kind::getaddrinfo_fail:
      msg = "Failed getaddrinfo() call";
      break;
    case kind::no_socket_found:
      msg = "Was not able to find a viable socket";
      break;
    case kind::uninitialized_socket:
      msg = "Trying to use -1 socket";
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
