#include <iostream>

#include "../error.h"
#include "../socket.h"
#include "config.h"
#include "server.h"

// TODO add stopping on signal?
int main(int argc, char* argv[]) {
  try {
    server::config_t conf(argc, argv);
    socket_t sockfd(std::to_string(conf.get_port_num()).c_str());
    server::server_t s(std::move(sockfd), conf);
    while (true) {
      s.wait_for_start();
      s.play_game();
    }
  } catch (const server::server_parameter_error& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const critical_error& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
