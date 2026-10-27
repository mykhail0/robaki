
#include "../socket.h"
#include "config.h"
#include "server.h"

// TODO add catching exceptions (from_client)
int main(int argc, char* argv[]) {
  try {
    server::config_t conf(argc, argv);
    socket_t sockfd(std::to_string(conf.port_num).c_str());
    server::server_t s(std::move(sockfd), conf.turning_speed, conf.width,
                       conf.height, conf.seed, conf.rounds_per_sec);
    while (true) {
      s.wait_for_start();
      s.play_game();
    }
  } catch (const server::server_parameter_error& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  } catch (const socket_error& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
