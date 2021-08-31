#ifndef CONFIG_H
#define CONFIG_H

#include "../consts.h"

// Class for checking command line arguments
// and extracting server parameters.
struct Config {
    uint16_t port_num;
    uint32_t seed;
    int rounds_per_sec;
    int turning_speed;
    dim_t width;
    dim_t height;

    Config(int, char *argv[]);
};

#endif /* CONFIG_H */
