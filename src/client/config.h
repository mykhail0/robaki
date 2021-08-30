#ifndef CLIENT_CONFIG_H
#define CLIENT_CONFIG_H

#include <cstdint>

struct ClientConfig {
    std::string game_server;
    std::string player_name;
    uint16_t port_num;
    std::string gui;
    uint16_t gui_port;

    ServerConfig(int, char *argv[]);
};

#endif /* CLIENT_CONFIG_H */
