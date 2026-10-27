#include "timer.h"

#include <sys/timerfd.h>
#include <unistd.h>

#include "error.h"

timer_fd_t::timer_fd_t(long nsec)
    : fd(timerfd_create(CLOCK_REALTIME, TFD_NONBLOCK)) {
  itimerspec val;
  val.it_value.tv_sec = 0;
  val.it_value.tv_nsec = nsec;
  val.it_interval = val.it_value;
  if (timerfd_settime(get(), 0, &val, nullptr) != 0) {
    throw critical_error(critical_error::kind::failed_timer);
  }
}

uint64_t timer_fd_t::read_timer() const {
  uint64_t exp = 0;
  if (read(fd, &exp, sizeof exp) == -1) {
    perror("Error reading from timer.\n");
  }
  return exp;
}

timer_fd_t::~timer_fd_t() {
  if (fd != -1) {
    if (close(fd) != 0) {
      perror("close(fd)");
    }
  }
}
