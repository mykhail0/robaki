#ifndef UTILITY_H
#define UTILITY_H

// Some useful functions and classes for client and server packages.

#include "consts.h"
#include <memory>
#include <poll.h>
#include <netinet/in.h>
#include <string>

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
// bool true iff initial interval should be too.
void settimer(int, long nsec, bool);

class Serializable {
public:
    virtual byte_t const *data() const = 0;
    virtual size_t size() const = 0;
};

// Receives some bytes from a given socket. Returns how many bytes.
ssize_t rcv_msg(int, sockaddr_storage *, socklen_t *, void *, size_t);

// Wrapper class for sockaddr_storage.
struct SockAddr {
    std::unique_ptr<sockaddr_storage> addr;
    size_t sz;

protected:
    void swap(SockAddr &);

public:
    SockAddr() : addr(nullptr), sz(0) {}
    SockAddr(sockaddr_storage const *, size_t);
    SockAddr(const SockAddr &x) : SockAddr(x.get_addr(), x.size()) {}
    SockAddr &operator=(SockAddr);

    // For use in copy ctor.
    sockaddr_storage const *get_addr() const { return addr.get(); }
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
T BEbytes2num(byte_t *bytes) {
    T ret = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        // There are 8 bits in a byte.
        ret <<= 8;
        ret |= static_cast<T>(bytes[i]);
    }
    return ret;
}

// Same as std::stoll() except it throws std::invalid_argument
// if there are non digits in the string.
long long stol_wrap(const std::string &);

#endif /* UTILITY_H */
