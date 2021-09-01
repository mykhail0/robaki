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
            case 'p': {
                auto t = stol_wrap(optarg);
                check_port(t);
                port_num = t;
                } break;
            case 's': {
                auto t = stol_wrap(optarg);
                check_num(t, 0, UINT32_MAX, "Negative seed.", "Too large seed.");
                seed = t;
                } break;
            case 't': {
                auto t = stol_wrap(optarg);
                check_num(t, 1, 90, "Nonpositive turning speed.", "Too large turning speed.");
                turning_speed = t;
                } break;
            case 'v': {
                auto t = stol_wrap(optarg);
                check_num(t, 1, 250, "Nonpositive number of rounds per second.", "Too large number of rounds per second.");
                rounds_per_sec = t;
                } break;
            // 720p restrictions.
            case 'w': {
                auto t = stol_wrap(optarg);
                check_num(t, 16, 2048, "Too small width.", "Too large width.");
                width = t;
                } break;
            case 'h': {
                auto t = stol_wrap(optarg);
                check_num(t, 16, 2048, "Too small height.", "Too large height.");
                height = t;
                } break;
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
