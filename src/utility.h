#ifndef UTILITY_H
#define UTILITY_H

// Some useful functions and classes for both client and server.

#include <netinet/in.h>
#include <poll.h>

#include <memory>
#include <string>

#include "consts.h"

// Throws std::out_of_range exceptions if the first argument does not lie in the
// range [second, third argument]. Error messages that go with the exception are
// parametrized by argument strings.
void check_num(long long, long long, long long, const std::string&,
               const std::string&);

// Caller for check_num() specialized for checking port numbers.
void check_port(long);

// Zeroes revents field for every member of pollfd array.
template <size_t N>
void reset_events(std::array<pollfd, N> fds) {
  for (auto& fd : fds) {
    fd.revents = 0;
  }
}

class serializable {
 public:
  virtual byte_t const* data() const = 0;
  virtual size_t size() const = 0;
};

// Wrapper class for sockaddr_storage.
struct SockAddr {
  std::unique_ptr<sockaddr_storage> addr;
  size_t sz;

 protected:
  void swap(SockAddr&);

 public:
  SockAddr() : addr(nullptr), sz(0) {}
  SockAddr(sockaddr_storage const*, size_t);
  SockAddr(const SockAddr& x) : SockAddr(x.get_addr(), x.size()) {}
  SockAddr& operator=(SockAddr);

  // For use in copy ctor.
  sockaddr_storage const* get_addr() const { return addr.get(); }
  size_t size() const { return sz; }

  // For usage in std::map as a key.
  bool operator<(const SockAddr&) const;
  bool operator==(const SockAddr&) const;
};

// Function for deserialization.
// Converts big endian bytes to a number.
// Used with T as byte_t, uint32_t and uint64_t
// instead of ntohl() and be64toh().
// Reason for this is choosing serialization over casting structs.
template <class T>
T BEbytes_to_num(byte_t* bytes) {
  T ret = 0;
  for (size_t i = 0; i < sizeof(T); ++i) {
    // There are 8 bits in a byte.
    ret <<= 8;
    ret |= static_cast<T>(bytes[i]);
  }
  return ret;
}

// Same as std::stol() except it throws std::invalid_argument if there are non
// digits in the string.
long stol_wrap(const std::string&);

#endif /* UTILITY_H */
