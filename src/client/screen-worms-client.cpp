#include "config.h"
#include "client.h"
#include "../err.h"

#include <netdb.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/timerfd.h>

#include <cstring>

// Returns a UDP socket file descriptor for the client to use.
int setup_UDP(const std::string &server, uint16_t port_num, SockAddr &srvr) {
    int sockfd = -1;

    addrinfo hints, *addr_list;
    memset(&hints, 0, sizeof hints);
    // UDP/(IPv4|IPv6)
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if (getaddrinfo(server.c_str(), std::to_string(port_num).c_str(), &hints, &addr_list) != 0)
        syserr("getaddrinfo");

    for (addrinfo *addr_ptr = addr_list; addr_ptr != nullptr; addr_ptr = addr_ptr->ai_next) {
        if ((sockfd = socket(addr_ptr->ai_family, addr_ptr->ai_socktype, addr_ptr->ai_protocol)) < 0) {
            perror("socket");
            continue;
        }

        srvr = SockAddr((sockaddr_storage *) addr_ptr->ai_addr, addr_ptr->ai_addrlen);
        break;
    }

    freeaddrinfo(addr_list);

    if (sockfd < 0)
        fatal("Valid socket not found.\n");

    // Add fd to file descriptors closed atexit().
    add_fd(sockfd);

    // Non blocking mode.
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0)
        syserr("fcntl");

    return sockfd;
}

// Returns a TCP socket file descriptor for the client to use.
int setup_TCP(const std::string &server, uint16_t port_num) {
    int sockfd = -1, yes = 1;

    addrinfo hints, *addr_list;
    memset(&hints, 0, sizeof hints);
    // TCP/(IPv4|IPv6)
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(server.c_str(), std::to_string(port_num).c_str(), &hints, &addr_list) != 0)
        syserr("getaddrinfo");

    for (addrinfo *addr_ptr = addr_list; addr_ptr != nullptr; addr_ptr = addr_ptr->ai_next) {
        if ((sockfd = socket(addr_ptr->ai_family, addr_ptr->ai_socktype, addr_ptr->ai_protocol)) < 0) {
            perror("socket");
            continue;
        }

        // Turn off Nagle algorithm.
        if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof yes) != 0) {
            perror("setsockopt");
            close_err(sockfd);
            continue;
        }

        if (connect(sockfd, addr_ptr->ai_addr, addr_ptr->ai_addrlen) < 0) {
            perror("connect");
            close_err(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(addr_list);

    if (sockfd < 0)
        fatal("Valid socket not found.\n");

    // Add fd to file descriptors closed atexit().
    add_fd(sockfd);

    // Non blocking mode.
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0)
        syserr("fcntl");

    return sockfd;
}

constexpr int POLL_N = 3,
    GAME = 0,
    GUI = 1,
    TIME = 2;

void client(Client &c) {
    pollfd polled_fd[POLL_N];

    polled_fd[GAME].fd = c.game();

    polled_fd[GUI].fd = c.GUI();
    polled_fd[GUI].events = POLLOUT;

    polled_fd[TIME].fd = timerfd_create(CLOCK_REALTIME, O_NONBLOCK);

    for (int i = 0; i < POLL_N; ++i)
        polled_fd[i].events |= POLLIN;
    zero_revents(polled_fd, POLL_N);

    settimer(polled_fd[TIME].fd, 30000000L);
    while (true) {
        int ret = poll(polled_fd, POLL_N, -1);
        if (ret == 0) {
            perror("Timeout reached.");
        } else if (ret < 0) {
            perror("Error in poll().");
        } else {
            // TODO POLLERR etc.
            if (polled_fd[GAME].revents & POLLIN)
                c.receive_from_game();

            if (polled_fd[GUI].revents & POLLIN) {
                c.receive_from_gui();
                polled_fd[GUI].events |= POLLOUT;
            }

            if (polled_fd[GUI].revents & POLLOUT) {
                if (c.send_to_gui())
                    polled_fd[GUI].events = POLLIN;
            }

            if (polled_fd[TIME].revents & POLLIN)
                c.send_to_game();
        }

        zero_revents(polled_fd, POLL_N);
    }
}

int main(int argc, char *argv[]) {
    if (std::atexit(atexit_clean_up) != 0)
        syserr("atexit()");
    Config conf(argc, argv);
    SockAddr game_srvr;
    int game_sockfd = setup_UDP(conf.game_server, conf.port_num, game_srvr);
    int gui_sockfd = setup_TCP(conf.gui, conf.gui_port);
    Client c(conf.player_name, gui_sockfd, game_sockfd, game_srvr);
    client(c);
    return 0;
}
