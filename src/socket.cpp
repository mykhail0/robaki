#include "socket.h"

#include <asm-generic/socket.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <unistd.h>

#include "error.h"

socket_t::anyIP_ainfo_t::anyIP_ainfo_t(
    socket_t::anyIP_ainfo_t::transport_layer_protocol_t protocol,
    bool is_server)
    : ai() {
  ai.ai_family = is_server ? AF_INET6 : AF_UNSPEC;
  switch (protocol) {
    case transport_layer_protocol_t::UDP:
      ai.ai_socktype = SOCK_DGRAM;
      break;
    case transport_layer_protocol_t::TCP:
      ai.ai_socktype = SOCK_STREAM;
      break;
  }
  ai.ai_flags = AI_NUMERICSERV;
  if (is_server) {
    ai.ai_flags |= AI_PASSIVE;
  }
}

socket_t::addr_list_t::addr_list_t(const char* addr, uint16_t port,
                                   const anyIP_ainfo_t& hints) {
  int retval =
      getaddrinfo(addr, std::to_string(port).c_str(), &hints.ai, &addr_list);
  if (retval != 0) {
    throw critical_error(critical_error::kind::getaddrinfo_fail,
                         gai_strerror(retval));
  }
}

socket_t::addr_list_t::addr_list_t(const std::string& addr, uint16_t port,
                                   const anyIP_ainfo_t& hints)
    : addr_list_t(addr.c_str(), port, hints) {}

socket_t::addr_list_t::addr_list_t(uint16_t port, const anyIP_ainfo_t& hints)
    : addr_list_t(nullptr, port, hints) {}

socket_t::addr_list_t::~addr_list_t() { freeaddrinfo(addr_list); }

socket_t::socket_t(int sockfd) : sockfd(sockfd) {
  if (sockfd == -1) {
    throw critical_error::kind::uninitialized_socket;
  }
}

socket_t::socket_t(const std::string& server, uint16_t port_num,
                   sock_addr_t& server_socket)
    : sockfd(-1) {
  anyIP_ainfo_t ai(anyIP_ainfo_t::transport_layer_protocol_t::UDP, false);

  addr_list_t addr_list(server, port_num, ai);
  bool found = false;
  for (addrinfo* addr_ptr = addr_list.addr_list;
       not found and addr_ptr != nullptr; addr_ptr = addr_ptr->ai_next) {
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
      server_socket = sock_addr_t((sockaddr_storage*)addr_ptr->ai_addr,
                                  addr_ptr->ai_addrlen);
      found = true;
    }
  }

  if (not found) {
    throw critical_error(critical_error::kind::no_socket_found);
  }
}

socket_t::socket_t(const std::string& server, uint16_t port_num) : sockfd(-1) {
  anyIP_ainfo_t ai(anyIP_ainfo_t::transport_layer_protocol_t::TCP, false);
  addr_list_t addr_list(server, port_num, ai);
  bool found = false;
  for (addrinfo* addr_ptr = addr_list.addr_list;
       not found and addr_ptr != nullptr; addr_ptr = addr_ptr->ai_next) {
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
  if (not found) {
    throw critical_error(critical_error::kind::no_socket_found);
  }
}

socket_t::socket_t(uint16_t port_num) : sockfd(-1) {
  anyIP_ainfo_t ai(anyIP_ainfo_t::transport_layer_protocol_t::UDP, true);
  addr_list_t addr_list(port_num, ai);

  bool found = false;
  for (addrinfo* addr_ptr = addr_list.addr_list;
       not found and addr_ptr != nullptr; addr_ptr = addr_ptr->ai_next) {
    int sockfd_ = socket(addr_ptr->ai_family, addr_ptr->ai_socktype,
                         addr_ptr->ai_protocol);
    if (sockfd_ < 0) {
      perror("socket");
      continue;
    }
    socket_t tmp(sockfd_);
    int no = 0;
    if (setsockopt(tmp.get(), IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof no) != 0) {
      perror("setsockopt(IPV6_6ONLY)");
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
  if (not found) {
    throw critical_error(critical_error::kind::no_socket_found);
  }
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
    throw critical_error(critical_error::kind::uninitialized_socket);
  }
  return sockfd;
}

void socket_t::send(const sock_addr_t& dest_addr, const byte_t* data,
                    size_t len) {
  if (sendto(get(), data, len, 0, dest_addr.get_addr(), dest_addr.size()) ==
      -1) {
    perror("sendto error");
  }
}

void socket_t::close_socket() {
  if (sockfd != -1) {
    if (close(sockfd) != 0) {
      perror("close(sockfd)");
    }
    sockfd = -1;
  }
}
