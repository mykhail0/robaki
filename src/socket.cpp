#include "socket.h"

#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <unistd.h>

socket_t::anyIP_ainfo_t::anyIP_ainfo_t(
    socket_t::anyIP_ainfo_t::transport_layer_protocol_t protocol,
    bool is_server)
    : ai() {
  ai.ai_family = AF_UNSPEC;
  switch (protocol) {
    case transport_layer_protocol_t::UDP:
      ai.ai_socktype = SOCK_DGRAM;
      break;
    case transport_layer_protocol_t::TCP:
      ai.ai_socktype = SOCK_STREAM;
      break;
  }
  // maybe not needed too? but then rewrite param exceptions to handle this
  ai.ai_flags = AI_NUMERICSERV;
  if (is_server) {
    ai.ai_flags |= AI_PASSIVE;
  }
}

std::string socket_error::format_message(kind ec, const std::string& detail) {
  std::string msg;
  switch (ec) {
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
      msg = "Unknown socket error";
      break;
  }
  if (not detail.empty()) {
    msg.append(": ").append(detail);
  }
  return msg;
}

socket_error::socket_error(kind ec, const std::string& detail)
    : std::runtime_error(format_message(ec, detail)), error_kind_(ec) {}

socket_t::socket_t(int sockfd) : sockfd(sockfd) {
  if (sockfd == -1) {
    throw socket_error::kind::uninitialized_socket;
  }
}

socket_t::socket_t(const std::string& server, uint16_t port_num,
                   SockAddr& server_socket)
    : sockfd(-1) {
  addrinfo* addr_list;
  anyIP_ainfo_t ai(anyIP_ainfo_t::transport_layer_protocol_t::UDP, false);

  int retval = getaddrinfo(server.c_str(), std::to_string(port_num).c_str(),
                           &ai.ai, &addr_list);
  if (retval != 0) {
    throw socket_error(socket_error::kind::getaddrinfo_fail,
                       gai_strerror(retval));
  }

  bool found = false;
  for (addrinfo* addr_ptr = addr_list; not found and addr_ptr != nullptr;
       addr_ptr = addr_ptr->ai_next) {
    int sockfd_ = socket(addr_ptr->ai_family, addr_ptr->ai_socktype,
                         addr_ptr->ai_protocol);
    if (sockfd_ < 0) {
      perror("socket");
      continue;
    }
    socket_t tmp(sockfd_);
    // Non blocking mode.
    if (fcntl(tmp.get(), F_SETFL, O_NONBLOCK) != -1) {
      *this = std::move(tmp);
      server_socket =
          SockAddr((sockaddr_storage*)addr_ptr->ai_addr, addr_ptr->ai_addrlen);
      found = true;
    }
  }

  freeaddrinfo(addr_list);
  if (not found) {
    throw socket_error(socket_error::kind::no_socket_found);
  }
}

socket_t::socket_t(const std::string& server, uint16_t port_num) : sockfd(-1) {
  addrinfo* addr_list;
  anyIP_ainfo_t ai(anyIP_ainfo_t::transport_layer_protocol_t::TCP, false);
  int retval = getaddrinfo(server.c_str(), std::to_string(port_num).c_str(),
                           &ai.ai, &addr_list);
  if (retval != 0) {
    throw socket_error(socket_error::kind::getaddrinfo_fail,
                       gai_strerror(retval));
  }
  bool found = false;
  for (addrinfo* addr_ptr = addr_list; not found and addr_ptr != nullptr;
       addr_ptr = addr_ptr->ai_next) {
    int sockfd_ = socket(addr_ptr->ai_family, addr_ptr->ai_socktype,
                         addr_ptr->ai_protocol);
    if (sockfd_ < 0) {
      perror("socket");
      continue;
    }
    socket_t tmp(sockfd_);
    // Turn off Nagle algorithm.
    int yes = 1;
    if (setsockopt(tmp.get(), IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes) !=
        0) {
      perror("setsockopt(TCP_NODELAY)");
      continue;
    }
    if (connect(tmp.get(), addr_ptr->ai_addr, addr_ptr->ai_addrlen) < 0) {
      perror("connect");
      continue;
    }
    // Non blocking mode.
    if (fcntl(tmp.get(), F_SETFL, O_NONBLOCK) != -1) {
      *this = std::move(tmp);
      found = true;
    }
  }
  freeaddrinfo(addr_list);
  if (not found) {
    throw socket_error(socket_error::kind::no_socket_found);
  }
}

socket_t::socket_t(const std::string& port_num) : sockfd(-1) {
  addrinfo* addr_list;
  anyIP_ainfo_t ai(anyIP_ainfo_t::transport_layer_protocol_t::UDP, true);
  int retval = getaddrinfo(nullptr, port_num.c_str(), &ai.ai, &addr_list);
  if (retval != 0) {
    throw socket_error(socket_error::kind::getaddrinfo_fail,
                       gai_strerror(retval));
  }

  bool found = false;
  for (addrinfo* addr_ptr = addr_list; not found and addr_ptr != nullptr;
       addr_ptr = addr_ptr->ai_next) {
    int sockfd_ = socket(addr_ptr->ai_family, addr_ptr->ai_socktype,
                         addr_ptr->ai_protocol);
    if (sockfd_ < 0) {
      perror("socket");
      continue;
    }
    socket_t tmp(sockfd_);
    int no = 0;
    if (setsockopt(tmp.get(), IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof no) != 0) {
      perror("setsockopt(IPV6_6ONLY no)");
      continue;
    }
    int yes = 1;
    if (setsockopt(tmp.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) !=
        0) {
      perror("setsockopt(SO_REUSEADDR)");
      continue;
    }
    if (setsockopt(tmp.get(), SOL_SOCKET, SO_REUSEPORT, &yes, sizeof yes) !=
        0) {
      perror("setsockopt(SO_REUSEPORT)");
      continue;
    }
    if (bind(tmp.get(), addr_ptr->ai_addr, addr_ptr->ai_addrlen) < 0) {
      perror("bind");
      continue;
    }
    // Non blocking mode.
    if (fcntl(tmp.get(), F_SETFL, O_NONBLOCK) != -1) {
      *this = std::move(tmp);
      found = true;
    }
  }
  freeaddrinfo(addr_list);
}

socket_t::socket_t(socket_t&& other) noexcept : sockfd(other.sockfd) {
  other.sockfd = -1;
}

socket_t& socket_t::operator=(socket_t&& other) noexcept {
  if (this != &other) {
    close_socket();
    sockfd = other.sockfd;
    other.sockfd = -1;
  }
  return *this;
}

socket_t::~socket_t() { close_socket(); }

int socket_t::get() const {
  if (sockfd == -1) {
    throw socket_error(socket_error::kind::uninitialized_socket);
  }
  return sockfd;
}

ssize_t socket_t::rcv_msg(sockaddr_storage* src_addr, socklen_t* addrlen,
                          void* buf, size_t buflen) const {
  ssize_t ret;
  if ((ret = recvfrom(get(), buf, buflen, 0, (sockaddr*)src_addr, addrlen)) <=
      0) {
    perror(ret == 0 ? "0 length datagram received" : "recvfrom error");
  }
  return ret;
}

void socket_t::close_socket() {
  if (sockfd != -1) {
    if (close(sockfd) != 0) {
      perror("close(sockfd)");
    }
    sockfd = -1;
  }
}
