#include "server.h"

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>

#include "../error.h"
#include "../timer.h"
#include "from_client.h"

server::server_t::server_t(socket_t&& sockfd, const config_t& config)
    : sockfd(std::move(sockfd)),
      rounds_per_sec(config.get_rps()),
      game(config.get_turning_speed(), config.get_width(), config.get_height(),
           std::make_unique<DeterministicRNG>(config.get_seed())) {}

std::tuple<bool, bool, bool> server::server_t::decide(
    client_pool_t::const_iterator it, const from_client_t& msg) {
  bool ignore, add, remove;
  if (it == clients.end()) {
    // New client address, so nothing to remove.
    remove = false;
    // We either ignore or add the new client.
    if (msg.player_name.empty() or
        (not clients.is_name_taken(msg.player_name) and
         clients.can_add_player())) {
      // Add only if empty name (observer) or there's still place for new
      // players.
      ignore = false;
      add = true;
    } else {
      ignore = true;
      add = false;
    }
  } else {
    if (it->second.id == msg.session_id) {
      // Good client shouldn't change the name.
      ignore = it->second.player_name != msg.player_name;
      remove = add = false;
    } else if (it->second.id < msg.session_id) {
      ignore = false;
      remove = add = true;
    } else {
      ignore = true;
      remove = add = false;
    }
  }
  return {ignore, add, remove};
}

void server::server_t::receive(bool update_worm) {
  // Receive a message.
  ssize_t len;
  sockaddr_storage client_address;
  socklen_t rcv_len = sizeof client_address;
  if ((len = sockfd.receive(&client_address, &rcv_len, buffer)) <= 0) {
    return;
  }
  // Try to process it.
  try {
    sock_addr_t sender(&client_address, rcv_len);
    from_client_t msg(buffer.data(), len);
    auto it = clients.find(sender);
    const auto [ignore, add, remove] = decide(it, msg);
    if (ignore) {
      return;
    }
    if (remove) {
      clients.remove_client(it);
    }
    bool empty_name = msg.player_name.empty();
    if (add) {
      client_t client(msg.session_id, std::move(msg.player_name),
                      msg.turn_direction);
      client.q.add(msg.next_expected_event_no);
      clients.add_client(std::move(sender), std::move(client));
    } else {
      it->second.q.add(msg.next_expected_event_no);
      bool became_ready = it->second.refresh(msg.turn_direction);
      if (became_ready) {
        ++clients.ready;
      }
    }
    if (update_worm and not add and not empty_name and
        it->second.worm_ind != -1) {
      game.update_worm(it->second.worm_ind, msg.turn_direction);
    }
  } catch (const parsing_error& e) {
    std::cerr << e.what() << std::endl;
  }
}

void server::server_t::play_round() {
  if (std::optional<uint32_t> first_event_ind = game.play_round()) {
    clients.broadcast(first_event_ind.value());
  }
}

void server::server_t::game_ready() {
  auto const [name_list, directions] = clients.game_ready();
  game.refresh(event_t(game.width, game.height, name_list), directions);
  clients.broadcast(0);
}

void server::server_t::wait_for_start() {
  std::array<pollfd, 2> polled_fd;
  constexpr size_t SOCKET = 0, IDLE_CLIENT = 1;

  polled_fd[SOCKET].fd = sockfd.get();
  polled_fd[SOCKET].events = POLLIN;
  if (nonempty_queue()) {
    polled_fd[SOCKET].events |= POLLOUT;
  }
  // Check for idleness every 0.05 s.
  timer_fd_t idle_timer(50000000L);
  polled_fd[IDLE_CLIENT].fd = idle_timer.get();
  polled_fd[IDLE_CLIENT].events = POLLIN;

  for (reset_events(polled_fd);
       clients.nonempty() < 2 or clients.ready != clients.nonempty();
       reset_events(polled_fd)) {
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

    if (polled_fd[IDLE_CLIENT].revents & POLLERR) {
      throw critical_error(critical_error::kind::timer_pollerr);
    }
    if (polled_fd[SOCKET].revents & POLLERR) {
      throw critical_error(critical_error::kind::socket_pollerr);
    }

    if (polled_fd[IDLE_CLIENT].revents & POLLIN) {
      idle_timer.read_timer();
      clients.remove_idle_clients();
    }

    if (polled_fd[SOCKET].revents & POLLOUT) {
      clients.send(sockfd, game);
    }

    if (polled_fd[SOCKET].revents & POLLIN) {
      receive();
    }

    polled_fd[SOCKET].events = POLLIN;
    if (nonempty_queue()) {
      polled_fd[SOCKET].events |= POLLOUT;
    }
  }

  game_ready();
}

void server::server_t::play_game() {
  std::array<pollfd, 3> polled_fd;
  constexpr size_t SOCKET = 0, IDLE_CLIENT = 1, ROUND = 2;

  timer_fd_t idle_timer(50000000L), round_timer(1000000000L / rounds_per_sec);

  polled_fd[SOCKET].fd = sockfd.get();
  // POLLOUT since there's an initial broadcast message to be sent.
  polled_fd[SOCKET].events = POLLOUT | POLLIN;
  polled_fd[IDLE_CLIENT].fd = idle_timer.get();
  polled_fd[IDLE_CLIENT].events = POLLIN;
  polled_fd[ROUND].fd = round_timer.get();
  polled_fd[ROUND].events = POLLIN;

  for (reset_events(polled_fd); not game.finished(); reset_events(polled_fd)) {
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
    if ((polled_fd[IDLE_CLIENT].revents & POLLERR) or
        (polled_fd[ROUND].revents & POLLERR)) {
      throw critical_error(critical_error::kind::timer_pollerr);
    }
    if (polled_fd[SOCKET].revents & POLLERR) {
      throw critical_error(critical_error::kind::socket_pollerr);
    }
    if (polled_fd[IDLE_CLIENT].revents & POLLIN) {
      idle_timer.read_timer();
      clients.remove_idle_clients();
    }

    if (polled_fd[ROUND].revents & POLLIN) {
      round_timer.read_timer();
      play_round();
    }

    if (polled_fd[SOCKET].revents & POLLOUT) {
      clients.send(sockfd, game);
    }

    if (polled_fd[SOCKET].revents & POLLIN) {
      receive(true);
    }

    polled_fd[SOCKET].events = POLLIN;
    if (nonempty_queue()) {
      polled_fd[SOCKET].events |= POLLOUT;
    }
  }
}
