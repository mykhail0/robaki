#ifndef SOCKET_H
#define SOCKET_H

#include <netdb.h>

#include <string>

#include "error.h"
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
           sock_addr_t& server_socket);
  // Construct a TCP socket connecting to a given server.
  socket_t(const std::string& server, uint16_t port_num);

  socket_t(const socket_t&) = delete;
  socket_t& operator=(const socket_t&) = delete;

  socket_t(socket_t&& other) noexcept;
  socket_t& operator=(socket_t&& other) noexcept;
  ~socket_t();

  int get() const;

  // Receive some bytes. Return how many bytes received.
  template <size_t N>
  ssize_t receive(sockaddr_storage* src_addr, socklen_t* addrlen,
                  std::array<byte_t, N>& buf) const {
    ssize_t ret;
    if ((ret = recvfrom(get(), buf.data(), buf.size(), 0, (sockaddr*)src_addr,
                        addrlen)) <= 0) {
      perror(ret == 0 ? "0 length datagram received" : "recvfrom error");
    }
    return ret;
  }
};

#endif  // SOCKET_H
