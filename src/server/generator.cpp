#include "generator.h"

uint32_t DeterministicRandGenerator::generate() {
    auto ret = seed;
    seed = (static_cast<unsigned long long>(seed) * 279410273) % 4294967291;
    return ret;
}
