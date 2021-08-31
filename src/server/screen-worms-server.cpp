#include "server.h"
#include "config.h"
#include "../err.h"

#include <netdb.h>
#include <fcntl.h>
#include <sys/timerfd.h>

#include <cstring>

// TODO optymalizacja sprawdzania idle clientow (mb kolejka priorytetowa)

// Returns a socket file descriptor the newly setup server is listening to.
int setup_server(const char *port_num) {
    int sockfd, bind_status, yes = 1;
    sockfd = bind_status = -1;

    addrinfo hints, *addr_list;
    memset(&hints, 0, sizeof hints);
    // UDP/(IPv4|IPv6)
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo(nullptr, port_num, &hints, &addr_list) != 0)
        syserr("getaddrinfo");

    for (addrinfo *addr_ptr = addr_list; addr_ptr != nullptr; addr_ptr = addr_ptr->ai_next) {
        if ((sockfd = socket(addr_ptr->ai_family, addr_ptr->ai_socktype, addr_ptr->ai_protocol)) < 0) {
            perror("socket");
            continue;
        }

        // For socket reusability.
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) != 0) {
            perror("setsockopt");
            close_err(sockfd);
            continue;
        }

        if ((bind_status = bind(sockfd, addr_ptr->ai_addr, addr_ptr->ai_addrlen)) < 0) {
            perror("bind");
            close_err(sockfd);
            continue;
        }

        break;
    }

    freeaddrinfo(addr_list);

    if (sockfd < 0 || bind_status < 0)
        fatal("Valid socket not found.\n");

    // Add fd to file descriptors closed atexit().
    add_fd(sockfd);

    // Non blocking mode.
    if (fcntl(sockfd, F_SETFL, O_NONBLOCK) < 0)
        syserr("fcntl");

    return sockfd;
}

constexpr int WAIT_POLL_N = 2,
    RUN_POLL_N = 3,
    SOCKET = 0,
    IDLE_CLIENT = 1,
    ROUND = 2;

constexpr int MIN_REQ_PLAYERS = 2;

// Function which exits when a round can be started.
void wait_for_start(Server &o) {
    /* o.names[player_name] = 1 if player_name player turned (is ready),
    -1 otherwise. */;
    // Number of ready players.
    size_t ready = 0;

    pollfd polled_fd[WAIT_POLL_N];

    polled_fd[SOCKET].fd = o.sockfd;
    polled_fd[SOCKET].events = POLLIN | (o.Qempty() ? 0 : POLLOUT);
    polled_fd[IDLE_CLIENT].fd = timerfd_create(CLOCK_REALTIME, O_NONBLOCK);
    polled_fd[IDLE_CLIENT].events = POLLIN;

    zero_revents(polled_fd, WAIT_POLL_N);

    /* If we check every client every 0.2 s then the longest
    period a client can stay idle is 2.2 which is a 10% relative error
    which is acceptable and allows us to check clients not too often. */
    settimer(polled_fd[IDLE_CLIENT].fd, 200000000L);

    // Until every connected player with nonempty name is ready
    // and there are enough of them.
    while (ready < MIN_REQ_PLAYERS || o.names.size() != ready) {
        int ret = poll(polled_fd, WAIT_POLL_N, -1);
        if (ret == 0) {
            perror("Timeout reached.");
        } else if (ret < 0) {
            perror("Error in poll() while waiting for a game.");
            continue;
        } else {
            // TODO POLLERR etc
            if (polled_fd[IDLE_CLIENT].revents & POLLIN) {
                // Time to check if there are any idle clients.
                o.check_for_idle_clients(ready);
            }

            if (polled_fd[SOCKET].revents & POLLOUT)
                o.send();

            if (polled_fd[SOCKET].revents & POLLIN)
                o.wait_receive(ready);
        }

        if (o.Qempty())
            polled_fd[SOCKET].events = POLLIN;
        else
            polled_fd[SOCKET].events |= POLLOUT;
    }

    o.game_ready();
}

void run(Server &o) {
    pollfd polled_fd[RUN_POLL_N];

    polled_fd[SOCKET].fd = o.sockfd;
    polled_fd[SOCKET].events = POLLOUT | POLLIN;
    polled_fd[IDLE_CLIENT].fd = timerfd_create(CLOCK_REALTIME, O_NONBLOCK);
    polled_fd[IDLE_CLIENT].events = POLLIN;
    polled_fd[ROUND].fd = timerfd_create(CLOCK_REALTIME, O_NONBLOCK);
    polled_fd[ROUND].events = POLLIN;

    zero_revents(polled_fd, RUN_POLL_N);

    settimer(polled_fd[IDLE_CLIENT].fd, 200000000L);
    settimer(polled_fd[ROUND].fd, 1000000000L / o.get_rounds_per_sec());

    // Until every connected player with nonempty name is ready
    // and there are enough of them.
    while (not o.done()) {
        int ret = poll(polled_fd, WAIT_POLL_N, -1);
        if (ret == 0) {
            perror("Timeout reached.");
        } else if (ret < 0) {
            perror("Error in poll() while waiting for a game.");
            continue;
        } else {
            // TODO POLLERR etc
            if (polled_fd[IDLE_CLIENT].revents & POLLIN)
                // Time to check if there are any idle clients.
                o.check_for_idle_clients();

            if (polled_fd[SOCKET].revents & POLLOUT)
                o.send();

            if (polled_fd[SOCKET].revents & POLLIN)
                o.run_receive();

            if (polled_fd[ROUND].revents & POLLIN)
                // Update game state.
                o.update_game();
        }

        if (o.Qempty())
            polled_fd[SOCKET].events = POLLIN;
        else
            polled_fd[SOCKET].events |= POLLOUT;
    }

    o.game_over();
}

int main(int argc, char *argv[]) {
    if (std::atexit(atexit_clean_up) != 0)
        syserr("atexit()");
    Config conf(argc, argv);
    int sockfd = setup_server(std::to_string(conf.port_num).c_str());
    Server s(sockfd, conf.turning_speed, conf.width, conf.height, conf.seed, conf.rounds_per_sec);
    while (true) {
        wait_for_start(s);
        run(s);
    }
    return 0;
}
