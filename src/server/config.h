#ifndef CONFIG_H
#define CONFIG_H

#include "../consts.h"

struct ServerConfig {
    uint16_t port_num;
    uint32_t seed;
    int rounds_per_sec;
    int turning_speed;
    dim_t width;
    dim_t height;

    ServerConfig(int, char *argv[]);

    /* Helper functions for ctor.
    Set values based on types returned by stol() and similar. */
    void set_port_num(long long);
    void set_seed(long long);
    void set_rounds_per_sec(int);
    void set_turning_speed(int);
    void set_width(long);
    void set_height(long);
};

#endif /* CONFIG_H */
