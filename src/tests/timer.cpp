#include "../utility.h"
#include <iostream>
#include <sys/timerfd.h>
#include <cassert>
#include <unistd.h>

int main() {
    pollfd polled_fd[1];

    auto ret = timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK);
    assert(ret != -1);

    polled_fd[0].fd = ret;
    polled_fd[0].events = POLLIN;

    zero_revents(polled_fd, 1);

    settimer(polled_fd[0].fd, 500000000L);
/*
                itimerspec y;
                timerfd_gettime(polled_fd[0].fd, &y);
                std::cerr << y.it_value.tv_sec << " " << y.it_value.tv_nsec << "\n";
*/

    int l = 0;
    while (true) {
        auto xd = poll(polled_fd, 1, -1);
        if (polled_fd[0].revents & POLLIN) {
            uint64_t exp;
            if (read(polled_fd[0].fd, &exp, sizeof exp) == -1)
                perror("Error reading from timer.\n");
            std::cout << l++ << std::endl;
        } else {
            std::cout << "wtf\n";
        }

    zero_revents(polled_fd, 1);
    }

    return 0;
}
