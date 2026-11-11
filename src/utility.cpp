#include "utility.h"

#include <sys/timerfd.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <stdexcept>

bool event_is_known(byte_t event_type) {
  return event_type <= static_cast<byte_t>(event_type_t::game_over);
}

void check_num(long long num, long long a, long long b, const std::string& low,
               const std::string& high) {
  assert(a <= b);
  if (num < a) {
    throw std::out_of_range(low);
  }
  if (num > b) {
    throw std::out_of_range(high);
  }
}

void check_port(long port) {
  check_num(port, 1, UINT16_MAX, "Nonpositive port number.",
            "Too large port number.");
}

void sock_addr_t::swap(sock_addr_t& x) {
  addr.swap(x.addr);
  std::swap(size_, x.size_);
}

sock_addr_t& sock_addr_t::operator=(sock_addr_t s) {
  s.swap(*this);
  return *this;
}

sock_addr_t::sock_addr_t(const sockaddr_storage* x, size_t size)
    : addr(std::make_unique<sockaddr_storage>()), size_(size) {
  if (x->ss_family != AF_INET and x->ss_family != AF_INET6) {
    throw std::invalid_argument("Unsupported address family.");
  }
  memcpy(addr.get(), x, size);
}

bool sock_addr_t::operator<(const sock_addr_t& y) const {
  bool ans = addr->ss_family == AF_INET;
  if (addr->ss_family == y.addr->ss_family) {
    // Same IP protocols.
    if (ans) {
      const sockaddr_in *a = (const sockaddr_in*)get_addr(),
                        *b = (const sockaddr_in*)y.get_addr();
      ans = a->sin_addr.s_addr == b->sin_addr.s_addr
                ? a->sin_port < b->sin_port
                : a->sin_addr.s_addr < b->sin_addr.s_addr;
    } else {
      const sockaddr_in6 *a = (const sockaddr_in6*)get_addr(),
                         *b = (const sockaddr_in6*)y.get_addr();
      int res = memcmp(a->sin6_addr.s6_addr, b->sin6_addr.s6_addr,
                       sizeof a->sin6_addr.s6_addr);
      ans = res == 0 ? a->sin6_port < b->sin6_port : res < 0;
    }
  }
  // If not same, then ours is "<" if it is IPv4.
  return ans;
}

bool sock_addr_t::operator==(const sock_addr_t& y) const {
  return not(*this < y and y < *this);
}

long stol_wrap(const std::string& s) {
  if (s.find_first_not_of("0123456789") != std::string::npos) {
    std::string what = "Non digits in a number string: ";
    what.append(s);
    throw std::invalid_argument(what);
  }
  return stol(s);
}
