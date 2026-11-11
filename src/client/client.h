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
  sock_addr_t game_srvr;

  void send_to_game();

 public:
  client_t(const std::string&, socket_t&& gui_socket, socket_t&& game_socket,
           sock_addr_t&&);
  void loop();
};
}  // namespace client

#endif /* CLIENT_H */
