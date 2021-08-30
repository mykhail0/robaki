#include "config.h"
#include "../utility"
#include "../err.h"
#include <unistd.h>
#include <stdexcept>

ClientConfig::ClientConfig(int argc, char *argv[]) :
    // game_server, player_name, port_num, gui, gui_port
    // Default configuration.
    port_num(2021), gui("localhost"), gui_port(20210) {

    if (argc < 2)
        syserr("Too few arguments.");
    game_server = argv[1];

    int opt;
    while ((opt = getopt(argc + 1, argv[1], "n:p:i:r:")) != -1) {
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
                syserr("Usage: ./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]\n  * game_server – adres (IPv4 lub IPv6) lub nazwa serwera gry\n  * -n player_name – nazwa gracza, zgodna z opisanymi niżej wymaganiami\n  * -p n – port serwera gry (domyślne 2021)\n  * -i gui_server – adres (IPv4 lub IPv6) lub nazwa serwera obsługującego interfejs użytkownika (domyślnie localhost)\n  * -r n – port serwera obsługującego interfejs użytkownika (domyślnie 20210)\n");

            }
        } catch (const std::exception &e) {
            syserr(e.what());
        }
    }

    if (optind < argc)
        syserr("Nonoption arguments.");
}
