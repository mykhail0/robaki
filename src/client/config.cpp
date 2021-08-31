#include "config.h"
#include "../utility.h"
#include "../err.h"
#include <unistd.h>
#include <stdexcept>

Config::Config(int argc, char *argv[]) :
    // Default configuration.
    port_num(2021), gui("localhost"), gui_port(20210) {

    if (argc < 2)
        syserr("Too few arguments.");
    game_server = argv[1];

    int opt;
    while ((opt = getopt(argc - 1, argv + sizeof (char *), "n:p:i:r:")) != -1) {
        try {
            switch (opt) {
            case 'n':
                player_name = optarg;
                if (player_name.size() > 20)
                    throw std::length_error("Too long player name.");
                for (char c: player_name) {
                    if (c < 33 || 126 < c)
                        throw std::out_of_range("Invalid characters in player name.");
                }
                break;
            case 'p':
                check_port(port_num = std::stoll(optarg));
                break;
            case 'i':
                gui = optarg;
                break;
            case 'r':
                check_port(gui_port = std::stoll(optarg));
                break;
            default: /* '?' */
                syserr("Usage: ./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]\n  * game_server – adress (IPv4 lub IPv6) or game server name\n  * -n player_name – player name, 0-22 ASCII characters in range of [33, 126]\n  * -p n – game server port (2021 by default)\n  * -i gui_server – adress (IPv4 lub IPv6) or gui server name (localhost by default)\n  * -r n – gui server port (20210 by default)\n");

            }
        } catch (const std::exception &e) {
            syserr(e.what());
        }
    }

    if (optind < argc)
        syserr("Nonoption arguments.");
}
