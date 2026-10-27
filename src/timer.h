#ifndef TIMER_H
#define TIMER_H

#include <cstdint>

// RAII class for managing timer file descriptor.
class timer_fd_t {
 private:
  int fd = -1;

 public:
  // Construct a timer with `nsec` nanoseconds interval.
  explicit timer_fd_t(long nsec);
  timer_fd_t(const timer_fd_t& other) = delete;
  timer_fd_t& operator=(const timer_fd_t& other) = delete;
  ~timer_fd_t();
  int get() const { return fd; }
  uint64_t read_timer() const;
};

#endif  // TIMER_H
