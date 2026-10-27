#ifndef CLIENT_H
#define CLIENT_H

#include "../socket.h"
#include "from_game.h"
#include "from_gui.h"

namespace client {
class client_t {
 private:
  session_id_t id;
  std::string player_name;

  game_to_gui_t buf;
  from_GUI_t gui;

  socket_t gui_sockfd;
  socket_t game_sockfd;
  SockAddr game_srvr;

  void receive_from_game() { buf.recv(game_sockfd, game_srvr); }
  void send_to_game();
  void receive_from_gui() { gui.receive(gui_sockfd); }
  bool send_to_gui() { return buf.send(gui_sockfd); }

 public:
  client_t(const std::string&, socket_t&& gui_socket, socket_t&& game_socket,
           const SockAddr&);
  void loop();
};
}  // namespace client

#endif /* CLIENT_H */
