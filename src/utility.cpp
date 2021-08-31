#include "utility.h"
#include "err.h"
#include <unordered_set>
#include <cstring>
#include <unistd.h>
#include <sys/timerfd.h>

namespace {
    // List of file descriptors which are closed by atexit_clean_up()
    // and where fds from add_fd() are added.
    std::unordered_set<int> fds;
}

void check_num(long long num, long long a, long long b, const std::string &low, const std::string &high) {
    if (num < a)
        throw std::out_of_range(low);
    if (num > b)
        throw std::out_of_range(high);
}

void check_port(long long port) {
    check_num(port, 1, UINT16_MAX, "Nonpositive port number.", "Too large port number.");
}

void zero_revents(pollfd *fd, int n) {
    for (int i = 0; i < n; ++i)
        fd[i].revents = 0;
}

void close_err(int fd) {
    if (fd == -1)
        return;
    if (close(fd) != 0)
        perror("close");
}

void add_fd(int fd) {
    fds.insert(fd);
}

void atexit_clean_up() {
    for (auto fd: fds)
        close_err(fd);
}

void settimer(int fd, long nsec) {
    itimerspec val;
    val.it_value.tv_sec = 0;
    val.it_value.tv_nsec = nsec;
    val.it_interval = val.it_value;
    if (timerfd_settime(fd, 0, &val, nullptr) != 0)
        syserr("timerfd_settime()");
}

ssize_t rcv_msg(int sockfd, sockaddr_storage *src_addr, socklen_t *addrlen, void *buf, size_t buflen) {
    ssize_t ret;
    if ((ret = recvfrom(sockfd, buf, buflen, 0,
        (sockaddr *) src_addr, addrlen)) <= 0)
        perror(ret == 0 ? "0 length datagram received" : "recvfrom error");
    return ret;
}

void SockAddr::swap(SockAddr &x) {
    addr.swap(x.addr);
    std::swap(sz, x.sz);
}

SockAddr &SockAddr::operator=(SockAddr s) {
    s.swap(*this);
    return *this;
}

SockAddr::SockAddr(sockaddr_storage const *x, size_t sz) : addr(std::make_unique<sockaddr_storage>()), sz(sz) {
    if (x->ss_family != AF_INET && x->ss_family != AF_INET6)
        throw std::invalid_argument("Strange address family.");
    memcpy(addr.get(), x, sizeof(sockaddr_storage));
}

bool SockAddr::operator<(const SockAddr &y) const {
    bool ans = addr->ss_family == AF_INET;
    if (addr->ss_family == y.addr->ss_family) {
        // Same ip protocols.
        if (ans) {
            const sockaddr_in *a = (const sockaddr_in *) get_addr(),
                *b = (const sockaddr_in *) y.get_addr();
            ans = a->sin_addr.s_addr == b->sin_addr.s_addr ?
                a->sin_port < b->sin_port :
                a->sin_addr.s_addr < b->sin_addr.s_addr;
        } else {
            const sockaddr_in6 *a = (const sockaddr_in6 *) get_addr(),
                *b = (const sockaddr_in6 *) y.get_addr();
            int res = memcmp(a->sin6_addr.s6_addr, b->sin6_addr.s6_addr, sizeof a->sin6_addr.s6_addr);
            ans = res == 0 ? a->sin6_port < b->sin6_port : res < 0;
        }
    }
    // If not same, then ours is "<" if it is IPv4.
    return ans;
}

bool SockAddr::operator==(const SockAddr &y) const {
    return !(*this < y && y < *this);
    // FIXME faster. But idk if correct.
    // return memcmp(&addr, y.get_addr(), sizeof addr) == 0 && size() == y.size();
}
