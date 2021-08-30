// TODO optymalizacja sprawdzania idle clientow (mb kolejka priorytetowa)

constexpr int WAIT_POLL_N = 2;
constexpr int RUN_POLL_N = 3;
constexpr int SOCKET = 0;
constexpr int IDLE_CLIENT = 1;
constexpr int ROUND = 2;

constexpr int MIN_REQ_PLAYERS = 2;

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

// Check for idle clients and remove those which are idle.
// Called in wait_for_start().
void check_for_idle_clients_WAIT(Server &o, size_t &ready) {
   for (auto it = o.clients.cbegin(); it != o.clients.cend(); ) {
        if (std::chrono::system_clock::now() > it->second.deadline) {
            auto it2 = o.names.find(it->second.player_name);
            if (it2 != o.names.end()) {
                ready -= it2->second == NAMES_READY;
                o.names.erase(it2);
            }
            it = clients.erase(it);
        } else {
            ++it;
        } 
    }
}

// Function which exits when a round can be started.
void wait_for_start(Server &o) {
    /* o.names[player_name] = 1 if player_name player turned (is ready),
    -1 otherwise. */;
    // Number of ready players.
    size_t ready = 0;

    byte_t buffer[MAX_2_SRVR_MSG_LEN];
    ssize_t len;

    pollfd polled_fd[WAIT_POLL_N];

    polled_fd[SOCKET].fd = sockfd;
    polled_fd[IDLE_CLIENT].fd = timerfd_create(CLOCK_REALTIME, O_NONBLOCK);

    for (int i = 0; i < WAIT_POLL_N; ++i)
        polled_fd[i].events |= POLLIN;
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
                check_for_idle_clients_WAIT();
            }

            if (polled_fd[SOCKET].revents & POLLOUT)
                o.send();

            if (polled_fd[SOCKET].revents & POLLIN)
                wait_receive(s, buffer, len, MAX_2_SRVR_MSG_LEN, ready);
        }

        if (o.Qempty())
            polled_fd[SOCKET].events = POLLIN;
        else
            polled_fd[SOCKET].events |= POLLOUT;
    }
}

/*
// Ignore SIGPIPE.
void block_signals() {
    struct sigaction action;
    sigset_t set, block_mask;
    if (sigemptyset(&set) == -1)
        syserr("sigemptyset");
    if (sigaddset(&set, SIGPIPE) == -1)
        syserr("sigaddset");
    if (sigemptyset(&block_mask) == -1)
        syserr("sigemptyset");
    action.sa_handler = SIG_IGN;
    action.sa_mask = block_mask;
    action.sa_flags = 0;
    if (sigaction(SIGPIPE, &action, NULL) == -1)
        syserr("sigaction");
}
*/


int main(int argc, char *argv[]) {
    if (std::atexit(atexit_clean_up) != 0)
        syserr("atexit()");
    ServerConfig conf(argc, argv);
    // block_signals();
    int sockfd = setup_server(std::to_string(conf.port_num).c_str());
    Server s(sockfd, conf.turning_speed, conf.width, conf.height, conf.seed);
    wait_for_start(s);
    return 0;
}
