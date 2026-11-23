#include "client.h"

#include <chrono>

#include "../timer.h"
#include "to_game.h"

client::client_t::client_t(const std::string& player_name,
                           socket_t&& gui_sockfd, socket_t&& game_sockfd)
    : id(std::chrono::duration_cast<std::chrono::milliseconds>(
             std::chrono::system_clock::now().time_since_epoch())
             .count()),
      player_name(player_name),
      gui_sockfd(std::move(gui_sockfd)),
      game_sockfd(std::move(game_sockfd)) {}

void client::client_t::send_to_game() {
  to_game_t msg(id, gui.turn_direction(), buf.next_expected_event_no(),
                player_name);
  game_sockfd.send_data(msg.data(), msg.size());
}

void client::client_t::loop() {
  std::array<pollfd, 3> polled_fd;
  static constexpr size_t GAME = 0, GUI = 1, TIME = 2;
  timer_fd_t timer_fd(30000000L);

  polled_fd[GAME].fd = game_sockfd.get();
  polled_fd[GAME].events = POLLIN;
  polled_fd[GUI].fd = gui_sockfd.get();
  polled_fd[GUI].events = POLLIN;
  polled_fd[TIME].fd = timer_fd.get();
  polled_fd[TIME].events = POLLIN;

  for (reset_events(polled_fd); true; reset_events(polled_fd)) {
    int ret = -1;
    while (ret < 0) {
      if (-1 == (ret = poll(polled_fd.data(), polled_fd.size(), -1)) and
          errno == EINVAL) {
        throw critical_error(critical_error::kind::poll_error);
      }
    }
    // Cannot timeout.
    if (ret == 0) {
      throw critical_error(critical_error::kind::unexpected_poll_timeout);
    }
    if (polled_fd[TIME].revents & POLLERR) {
      throw critical_error(critical_error::kind::timer_pollerr);
    }
    if ((polled_fd[GAME].revents & POLLERR) or
        (polled_fd[GUI].revents & POLLERR)) {
      throw critical_error(critical_error::kind::socket_pollerr);
    }

    if (polled_fd[GAME].revents & POLLIN) {
      buf.receive(game_sockfd);
    }

    if (polled_fd[GUI].revents & POLLIN) {
      gui.receive(gui_sockfd);
    }

    if (polled_fd[GUI].revents & POLLOUT) {
      buf.send(gui_sockfd);
    }

    if (polled_fd[TIME].revents & POLLIN) {
      timer_fd.read_timer();
      send_to_game();
    }

    polled_fd[GUI].events = POLLIN;
    if (not buf.empty()) {
      polled_fd[GUI].events |= POLLOUT;
    }
  }
}
