#ifndef UTILITY_H
#define UTILITY_H

#include "consts.h"
#include <poll.h>

// Throws std::out_of_range exceptions if the first argument does not lie in the range [second, third argument].
// Error messages that go with the exception are parametrized by argument strings.
void check_num(long long, long long, long long, const std::string &, const std::string &);

// Caller for check_num() specialized for checking port numbers.
void check_port(long long);

// Zeroes revents field for every member of pollfd array.
void zero_revents(pollfd *, int);

// Closes the file descriptor with the error message printed if needed.
void close_err(int);

// Adds the file descriptor to list of file descriptors closed if atexit_clean_up() is registered.
void add_fd(int);

// Function that closes file descriptors from a list.
void atexit_clean_up();

// Sets the interval of 'nsec' nanoseconds for the timer pointed to by the file descriptor.
void settimer(int, long nsec);

class Serializable {
    public:
    virtual byte_t *data() = 0;
    virtual size_t size() const = 0;
};

// Receives some bytes from a given socket. Returns how many bytes.
ssize_t rcv_msg(int, sockaddr_storage *, socklen_t *, void *, size_t);

// Wrapper class for sockaddr_storage.
struct SockAddr {
    sockaddr_storage addr;
    size_t sz;

    public:
    SockAddr(sockaddr_storage const *, size_t);
    SockAddr(const SockAddr &x) : SockAddr(x.get_addr(), x.size()) {}

    // For use in copy ctor.
    sockaddr_storage const *get_addr() const { return &addr; }
    size_t size() const { return sz; }

    // For usage in std::map as a key.
    bool operator<(const SockAddr &) const;
    bool operator==(const SockAddr &) const;
};

// Function for deserialization.
// Converts big endian bytes to a number.
// Used with T as byte_t, uint32_t and uint64_t
// instead of ntohl() and be64toh().
// Reason for this is choosing serialization over casting structs.
template <class T>
T BEbytes2num(byte_t *);

#endif /* UTILITY_H */
