#include "client_pool.h"

#include <array>
#include <cassert>
#include <cstring>

bool server::from_server_t::send(const socket_t& sockfd,
                                 const sock_addr_t& dest, const state_t& game) {
  if (empty(game)) {
    return false;
  }
  // TODO could cache this inside the struct potentially if the send fails
  std::array<byte_t, MAX_GAME_MSG_LEN> datagram;
  game_id_t id = htonl(game.game_id);
  uint32_t len = sizeof id;

  uint32_t start = ev_no;
  for (memcpy(datagram.data(), &id, len);
       ev_no < game.history.size() and
       game.history[ev_no].size() + len <= datagram.size();
       ++ev_no) {
    memcpy(datagram.data() + len, game.history[ev_no].data(),
           game.history[ev_no].size());
    len += game.history[ev_no].size();
  }

  ssize_t sent = sendto(sockfd.get(), datagram.data(), len, 0,
                        (const sockaddr*)dest.get_addr(), dest.size());
  if (sent == -1) {
    ev_no = start;
    if (errno != EWOULDBLOCK and errno != EAGAIN) {
      perror("Error sending from_server_t\n");
    }
    return false;
  }
  return true;
}

server::client_t::client_t(session_id_t id, const std::string& player_name,
                           turn_direction_t turn_direction)
    : id(id), player_name(player_name) {
  refresh(turn_direction);
}

bool server::client_t::refresh(turn_direction_t turn_direction) {
  deadline = std::chrono::system_clock::now() + std::chrono::milliseconds{2000};
  this->turn_direction = turn_direction;
  if (ready) {
    return false;
  } else {
    ready = turn_direction == turn_direction_t::right or
            turn_direction == turn_direction_t::left;
    return ready;
  }
}

bool server::client_pool_t::can_add_player() const {
  return names.size() < MAX_PLAYERS_NUM;
}

server::client_pool_t::iterator server::client_pool_t::find(
    const sock_addr_t& addr) {
  return clients.find(addr);
}

void server::client_pool_t::add_client(sock_addr_t&& client_addr,
                                       client_t&& client) {
  const auto [it, inserted] =
      clients.emplace(std::move(client_addr), std::move(client));
  assert(inserted);
  if (not it->second.player_name.empty()) {
    names.insert(it->second.player_name);
    if (it->second.ready) {
      ++ready;
    }
  }
}

server::client_pool_t::const_iterator server::client_pool_t::remove_client(
    const_iterator it) {
  if (it == end()) {
    return it;
  }
  if (not it->second.player_name.empty()) {
    names.erase(it->second.player_name);
    if (it->second.ready) {
      --ready;
    }
  }
  return clients.erase(it);
}

void server::client_pool_t::remove_client(const sock_addr_t& addr) {
  auto it = clients.find(addr);
  remove_client(it);
}

void server::client_pool_t::remove_idle_clients() {
  for (auto it = clients.cbegin(); it != clients.cend();) {
    if (std::chrono::system_clock::now() > it->second.deadline) {
      it = remove_client(it);
    } else {
      ++it;
    }
  }
}

bool server::client_pool_t::is_name_taken(const std::string& name) const {
  return names.count(name);
}

std::pair<std::string, std::vector<turn_direction_t>>
server::client_pool_t::game_ready() {
  std::vector<std::tuple<std::string_view, turn_direction_t, ssize_t*>>
      client_data;
  for (auto& [_, client] : clients) {
    if (client.player_name.empty()) {
      continue;
    }
    client_data.emplace_back(client.player_name.c_str(), client.turn_direction,
                             &(client.worm_ind));
    client.ready = false;
  }
  // Sort by names.
  std::sort(client_data.begin(), client_data.end());
  std::pair<std::string, std::vector<turn_direction_t>> ret;
  size_t i = 0;
  for (const auto& [s, dir, worm_ind_ptr] : client_data) {
    ret.first.append(s).append(1, '\0');
    ret.second.push_back(dir);
    *worm_ind_ptr = i++;
  }
  return ret;
}

void server::client_pool_t::send(const socket_t& sockfd, const state_t& s) {
  for (auto& [addr, client] : clients) {
    if (client.q.send(sockfd, addr, s)) {
      return;
    }
  }
}

void server::client_pool_t::broadcast(uint32_t start) {
  for (auto& [_, client] : clients) {
    client.q.add(start);
  }
}

bool server::client_pool_t::nonempty_queue(const state_t& game) const {
  for (const auto& [_, client] : clients) {
    if (not client.q.empty(game)) {
      return true;
    }
  }
  return false;
}

void server::client_pool_t::game_over() {
  ready = 0;
  for (auto& [_, client] : clients) {
    client.ready = false;
  }
}
