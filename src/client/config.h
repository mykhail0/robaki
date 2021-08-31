#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdint>

// Class for checking command line arguments
// and extracting client parameters.
struct Config {
    std::string game_server;
    std::string player_name;
    uint16_t port_num;
    std::string gui;
    uint16_t gui_port;

    Config(int, char *argv[]);
};

#endif /* CONFIG_H */
