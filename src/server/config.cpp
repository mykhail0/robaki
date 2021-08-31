#include "config.h"
#include "../utility.h"
#include "../err.h"
#include <ctime>
#include <unistd.h>

Config::Config(int argc, char *argv[]) :
    // Default configuration.
    port_num(2021), rounds_per_sec(50), turning_speed(6),
    width(640), height(480) {
    // Lower 32 bits.
    auto cur = time(NULL);
    seed = cur - ((cur >> 32) << 32);

    int opt;
    while ((opt = getopt(argc, argv, "p:s:t:v:w:h:")) != -1) {
        try {
            switch (opt) {
            case 'p':
                check_port(port_num = std::stoll(optarg));
                break;
            case 's':
                check_num(seed = std::stoll(optarg), 0, UINT32_MAX, "Negative seed.", "Too large seed.");
                break;
            case 't':
                check_num(turning_speed = std::stoi(optarg), 1, 90, "Nonpositive turning speed.", "Too large turning speed.");
                break;
            case 'v':
                check_num(rounds_per_sec = std::stoi(optarg), 1, 250, "Nonpositive number of rounds per second.", "Too large number of rounds per second.");
                break;
            // 720p restrictions.
            case 'w':
                check_num(width = std::stol(optarg), 16, 1280, "Too small width.", "Too large width.");
                break;
            case 'h':
                check_num(height = std::stol(optarg), 16, 720, "Too small height.", "Too large height.");
                break;
            default: /* '?' */
                syserr("Usage: ./screen-worms-server [-p n] [-s n] [-t n] [-v n] [-w n] [-h n]\n  * `-p n` – port number (`2021` by default)\n  * `-s n` – random number generator's seed (time(NULL) by default)\n  * `-t n` – integer which determines turning speed (6 by default)\n  * `-v n` – integer which determines game's pace (rounds per second, 50 by default)\n  * `-w n` – map's width in pixels (640 by default)\n  * `-h n` – map's height in pixels (480 by default)\n");
            }
        } catch (const std::exception &e) {
            syserr(e.what());
        }
    }

    if (optind < argc)
        syserr("Nonoption arguments.");
}
