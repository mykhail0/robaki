#include "server.h"

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <cstring>

#include "../timer.h"
#include "from_client.h"

namespace {
// Bits for the bitmask returned as part of the return type of
// Server::receive(). If REMOVED then also should be ADDED.
constexpr int IGNORED = 1, REMOVED = 2, ADDED = 4;
constexpr size_t SOCKET = 0, IDLE_CLIENT = 1, ROUND = 2;
}  // namespace

server::from_server_t::from_server_t(const std::stack<clientkey_t>& recipients,
                                     ssize_t start, ssize_t end)
    : recipients(recipients), ev_no(start), start(ev_no), end(end) {}

server::from_server_t::from_server_t(const clientkey_t& recipient,
                                     ssize_t ev_no)
    : ev_no(ev_no), start(ev_no), end(-1) {
  recipients.push(recipient);
}

int server::from_server_t::send(const server_t& s) {
  // Alias.
  auto& hist = s.game.history;

  ssize_t g = end == -1 ? hist.size() : end;
  bool tmp = hist.size() >= (size_t)g;
  if (not tmp) {
    perror(
        "New game already started, but need to send messages for a previous "
        "one! (Queue was not emptied of redundant messages)");
  }
  assert(tmp);

  while (not recipients.empty() and not s.contains(recipients.top())) {
    recipients.pop();
  }

  if (recipients.empty()) {
    return 0;
  }

  byte_t datagram[MAX_GAME_MSG_LEN];
  auto id = htonl(s.game.game_id);
  uint32_t len = sizeof id;

  auto prev_ev_no = ev_no;
  while (ev_no < g and hist[ev_no].size() + len <= MAX_GAME_MSG_LEN) {
    memcpy(datagram + len, hist[ev_no].data(), hist[ev_no].size());
    len += hist[ev_no++].size();
  }

  memcpy(datagram, &id, sizeof id);

  int ret = 0;
  if (len != sizeof id) {
    // If no events to be sent, then send nothing.
    ret = sendto(s.sockfd.get(), datagram, len, 0,
                 (sockaddr*)recipients.top().first.get_addr(),
                 recipients.top().first.size());

    if (ret == -1) {
      if (errno == EWOULDBLOCK or errno == EAGAIN) {
        ev_no = prev_ev_no;
      } else {
        perror("Error sening Server2Client\n");
      }
    }
  }

  if (ev_no >= g) {
    recipients.pop();
    ev_no = start;
  }

  return ret;
}

server::client_t::client_t(session_id_t id, const std::string& player_name)
    : id(id),
      deadline(std::chrono::system_clock::now() +
               std::chrono::milliseconds{2000}),
      player_name(player_name) {}

void server::client_t::update_deadline() {
  deadline = std::chrono::system_clock::now() + std::chrono::milliseconds{2000};
}

server::server_t::server_t(socket_t&& sockfd, int turning_speed, dim_t width,
                           dim_t height, uint32_t seed, int rounds_per_sec)
    : sockfd(std::move(sockfd)),
      rounds_per_sec(rounds_per_sec),
      game(state_t(turning_speed, width, height,
                   std::make_unique<DeterministicRNG>(seed))) {}

bool server::server_t::contains(const clientkey_t& key) const {
  auto it = clients.find(key.first);
  return it != clients.end() and it->second.id == key.second;
}

void server::server_t::add(const SockAddr& addr, session_id_t id,
                           const std::string& player_name) {
  clients.insert({addr, client_t(id, player_name)});
  if (not player_name.empty()) {
    names[player_name] = NAMES_DEFAULT;
  }
}

void server::server_t::remove(const SockAddr& addr) {
  auto it = clients.find(addr);
  names.erase(it->second.player_name);
  clients.erase(it);
  perror("Removed a client.\n");
}

void server::server_t::update(const SockAddr& addr) {
  auto it = clients.find(addr);
  if (it != clients.end()) {
    it->second.update_deadline();
  }
}

void server::server_t::add_msg(const clientkey_t& key, uint32_t ev_no) {
  q.push(from_server_t(key, ev_no));
}

void server::server_t::add_broadcast_msg(uint32_t start) {
  size_t end = game.history.size();
  std::stack<clientkey_t> recipients;
  for (auto const& it : clients) {
    recipients.push({it.first, it.second.id});
  }
  q.push(from_server_t(recipients, start, end));
}

void server::server_t::send() {
  if (q.empty()) {
    return;
  }
  q.front().send(*this);
  if (q.front().done()) {
    q.pop();
  }
}

std::pair<int, server::received_t> server::server_t::receive() {
  received_t ret;
  int stat = IGNORED;
  ssize_t len;

  sockaddr_storage client_address;
  socklen_t rcv_len = sizeof client_address;
  if ((len = sockfd.rcv_msg(&client_address, &rcv_len, buffer.data(),
                            buffer.size())) <= 0) {
    return {stat, ret};
  }

  try {
    SockAddr sender(&client_address, rcv_len);
    // Decipher the message.
    from_client_t msg(buffer.data(), len);

    auto client_it = clients.find(sender);
    auto name_it = names.find(msg.player_name);

    bool recognises_addr = client_it != clients.end(),
         recognises_name = name_it != names.end();

    bool must_add = not recognises_addr and not recognises_name,
         ignore = not recognises_addr and recognises_name;

    if (recognises_addr) {
      assert(must_add == ignore);
      assert(ignore == false);

      auto old_session_id = client_it->second.id;
      bool same_id_and_diff_name =
          old_session_id == msg.session_id and
          msg.player_name != client_it->second.player_name;

      must_add = old_session_id < msg.session_id and not recognises_name;
      ignore = old_session_id > msg.session_id or same_id_and_diff_name or
               (old_session_id < msg.session_id and recognises_name);

      if (must_add or same_id_and_diff_name) {
        ret.removed = client_it->second.player_name;
        remove(sender);
        stat |= REMOVED;
      }
    }

    // names.size() is the number of connected clients with non empty name.
    if (must_add and
        (not msg.player_name.empty() and names.size() == MAX_PLAYERS_NUM)) {
      must_add = false;
      ignore = true;
    }

    if (ignore) {
      // (Recognises the address, same session_id, different names) -> don't do
      // anything after removing this client. (Recognises the address, bigger
      // (newer) session_id, recognises the name) -> don't do anything after
      // removing this client. (Recognises the address, smaller session_id)
      // (Doesn't recognise the address, recognises the name)
      return {stat, ret};
    }

    stat &= (~IGNORED);

    if (must_add) {
      add(sender, msg.session_id, msg.player_name);
      stat |= ADDED;
    }

    update(sender);
    add_msg({sender, msg.session_id}, msg.next_expected_event_no);
    ret.turn_direction = msg.turn_direction;
    ret.player_name = msg.player_name;

  } catch (const parsing_error& e) {
    perror(e.what());
  }

  return {stat, ret};
}

void server::server_t::wait_receive(std::unordered_set<std::string>& ready) {
  auto [stat, ret] = receive();
  if (stat & REMOVED) {
    ready.erase(ret.removed);
  }
  if (stat & IGNORED) {
    return;
  }

  if (not ret.player_name.empty()) {
    auto it = ready.find(ret.player_name);
    if (it == ready.end() and ret.turn_direction != STRAIGHT) {
      ready.insert(ret.player_name);
    }
    names[ret.player_name] = ret.turn_direction;
  }
}

void server::server_t::run_receive() {
  auto [stat, ret] = receive();
  if (stat & (IGNORED | ADDED)) {
    return;
  }
  auto it = names.find(ret.player_name);
  if (it == names.end() or it->second == NAMES_DEFAULT) {
    return;
  }
  if (not done()) {
    game.update_worm(it->second, ret.turn_direction);
  }
}

void server::server_t::check_for_idle_clients(
    std::unordered_set<std::string>& ready) {
  for (auto it = clients.cbegin(); it != clients.cend();) {
    if (std::chrono::system_clock::now() > it->second.deadline) {
      // Remove an idle client.
      auto it2 = names.find(it->second.player_name);
      if (it2 != names.end()) {
        ready.erase(it->second.player_name);
        names.erase(it2);
      }
      it = clients.erase(it);
    } else {
      ++it;
    }
  }
}

void server::server_t::check_for_idle_clients() {
  for (auto it = clients.cbegin(); it != clients.cend();) {
    if (std::chrono::system_clock::now() > it->second.deadline) {
      // Remove an idle client.
      auto it2 = names.find(it->second.player_name);
      if (it2 != names.end()) {
        names.erase(it2);
      }
      it = clients.erase(it);
    } else {
      ++it;
    }
  }
}

void server::server_t::update_game() {
  auto [ind, sz] = game.increment();
  if (ind >= 0) {
    add_broadcast_msg(ind);
  }
}

void server::server_t::game_ready() {
  std::vector<std::string> sorted_names;
  for (const auto& it : names) {
    sorted_names.push_back(it.first);
  }
  std::sort(begin(sorted_names), end(sorted_names));
  std::string player_name_list;
  std::vector<byte_t> directions;
  size_t i = 0;
  for (const std::string& s : sorted_names) {
    player_name_list.append(s).append(1, '\0');
    directions.push_back(names[s]);
    names[s] = i++;
  }
  add_broadcast_msg(
      game.refresh(event_t(game.width, game.height, player_name_list.size(),
                           player_name_list.c_str()),
                   directions));
}

void server::server_t::game_over() {
  for (auto& it : names) {
    it.second = NAMES_DEFAULT;
  }
}

// Exit when a game can be started.
void server::server_t::wait_for_start() {
  // Ready players.
  std::unordered_set<std::string> ready;

  std::array<pollfd, 2> polled_fd;

  polled_fd[SOCKET].fd = sockfd.get();
  polled_fd[SOCKET].events = POLLIN;
  if (was_played()) {
    polled_fd[SOCKET].events |= (q.empty() ? 0 : POLLOUT);
  }
  // Check for idleness every 0.05 s.
  timer_fd_t idle_timer(50000000L);
  polled_fd[IDLE_CLIENT].fd = idle_timer.get();
  polled_fd[IDLE_CLIENT].events = POLLIN;

  reset_events(polled_fd);

  // Until every connected player with nonempty name is ready
  // and there are enough of them.
  // Minimum required number of players is 2.
  while (ready.size() < 2 or names.size() != ready.size()) {
    int ret = poll(polled_fd.data(), polled_fd.size(), -1);
    if (ret == 0) {
      perror("Timeout reached.");
    } else if (ret < 0) {
      perror("Error in poll() while waiting for a game.");
      continue;
    } else {
      // TODO POLLERR etc
      if (polled_fd[IDLE_CLIENT].revents & POLLIN) {
        idle_timer.read_timer();
        // Time to check if there are any idle clients.
        check_for_idle_clients(ready);
      }

      if (polled_fd[SOCKET].revents & POLLOUT) {
        send();
      }

      if (polled_fd[SOCKET].revents & POLLIN) {
        wait_receive(ready);
      }
    }

    polled_fd[SOCKET].events = POLLIN;
    if (was_played()) {
      polled_fd[SOCKET].events |= (q.empty() ? 0 : POLLOUT);
    }

    reset_events(polled_fd);
  }

  game_ready();
}

void server::server_t::play_game() {
  std::array<pollfd, 3> polled_fd;

  timer_fd_t idle_timer(50000000L),
      round_timer(1000000000L / get_rounds_per_sec());

  polled_fd[SOCKET].fd = sockfd.get();
  polled_fd[SOCKET].events = POLLOUT | POLLIN;
  polled_fd[IDLE_CLIENT].fd = idle_timer.get();
  polled_fd[IDLE_CLIENT].events = POLLIN;
  polled_fd[ROUND].fd = round_timer.get();
  polled_fd[ROUND].events = POLLIN;

  reset_events(polled_fd);

  // Until every connected player with nonempty name is ready
  // and there are enough of them.
  while (not done()) {
    int ret = poll(polled_fd.data(), polled_fd.size(), -1);
    if (ret == 0) {
      perror("Timeout reached.");
    } else if (ret < 0) {
      perror("Error in poll() while waiting for a game.");
      continue;
    } else {
      // TODO POLLERR etc
      if (polled_fd[IDLE_CLIENT].revents & POLLIN) {
        idle_timer.read_timer();
        // Time to check if there are any idle clients.
        check_for_idle_clients();
      }

      if (polled_fd[ROUND].revents & POLLIN) {
        round_timer.read_timer();
        // Update game state.
        update_game();
      }

      if (polled_fd[SOCKET].revents & POLLOUT) {
        send();
      }

      if (polled_fd[SOCKET].revents & POLLIN) {
        run_receive();
      }
    }

    if (q.empty()) {
      polled_fd[SOCKET].events = POLLIN;
    } else {
      polled_fd[SOCKET].events |= POLLOUT;
    }

    reset_events(polled_fd);
  }

  game_over();
}
