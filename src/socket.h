#ifndef SOCKET_H
#define SOCKET_H

#include <netdb.h>

#include <stdexcept>
#include <string>

#include "utility.h"

// RAII socket wrapper.
class socket_t {
 private:
  int sockfd = -1;
  // Close the file descriptor with an error message if needed.
  void close_socket();

  class anyIP_ainfo_t {
   public:
    enum class transport_layer_protocol_t { UDP, TCP };
    explicit anyIP_ainfo_t(transport_layer_protocol_t protocol, bool is_server);
    addrinfo ai;
  };

 public:
  // Construct a wrapper around a socket file descriptor (non -1).
  explicit socket_t(int sockfd);
  // Construct a bound UDP for a server on a given port.
  explicit socket_t(const std::string& port_num);
  // Construct a UDP socket connecting to a given server and save that server's
  // socket address into `server_socket`.
  socket_t(const std::string& server, uint16_t port_num,
           SockAddr& server_socket);
  // Construct a TCP socket connecting to a given server.
  socket_t(const std::string& server, uint16_t port_num);

  socket_t(const socket_t&) = delete;
  socket_t& operator=(const socket_t&) = delete;

  socket_t(socket_t&& other) noexcept;
  socket_t& operator=(socket_t&& other) noexcept;
  ~socket_t();

  int get() const;

  // Receive some bytes. Return how many bytes received.
  ssize_t rcv_msg(sockaddr_storage*, socklen_t*, void*, size_t) const;
};

class socket_error : public std::runtime_error {
 public:
  enum class kind { getaddrinfo_fail, no_socket_found, uninitialized_socket };

 private:
  kind error_kind_;
  static std::string format_message(kind ec, const std::string& detail);

 public:
  explicit socket_error(kind ec, const std::string& detail = "");
};

#endif  // SOCKET_H
