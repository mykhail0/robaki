#ifndef GENERATOR_H
#define GENERATOR_H

#include <cstdint>

class RandGenerator {
    protected:
    uint32_t seed;

    public:
    RandGenerator(uint32_t seed) : seed(seed) {}
    virtual uint32_t generate() = 0;
};

class DeterministicRandGenerator : public RandGenerator {
    public:
    DeterministicRandGenerator(uint32_t seed) : RandGenerator(seed) {}
    // r_0 = seed
    // r_i = (r_{i-1} * 279410273) mod 4294967291
    uint32_t generate();
};

#endif /* GENERATOR_H */
