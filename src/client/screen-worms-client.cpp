#include <iostream>

#include "../socket.h"
#include "client.h"
#include "config.h"
#include "runtime_exceptions.h"

int main(int argc, char* argv[]) {
  try {
    client::config_t conf(argc, argv);
    sock_addr_t game_srvr;
    socket_t game_socket(conf.game_server, conf.port_num, game_srvr),
        gui_socket(conf.gui, conf.gui_port);
    client::client_t c(conf.player_name, std::move(gui_socket),
                       std::move(game_socket), game_srvr);
    c.loop();
  } catch (const client::client_parameter_error& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const critical_error& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const client::parsing_error& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
