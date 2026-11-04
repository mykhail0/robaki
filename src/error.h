#ifndef ERROR_H
#define ERROR_H

#include <stdexcept>

class critical_error : public std::runtime_error {
 public:
  enum class kind {
    failed_send,
    failed_recv,
    failed_timer,
    timer_pollerr,
    socket_pollerr,
    unexpected_poll_timeout,
    poll_error,
    getaddrinfo_fail,
    no_socket_found,
    uninitialized_socket
  };

 private:
  kind error_kind_;
  static std::string format_message(kind ec, const std::string& detail);

 public:
  explicit critical_error(kind ec, const std::string& detail = "");
};

#endif  // ERROR_H
