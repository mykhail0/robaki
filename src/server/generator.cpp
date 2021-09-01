#include "generator.h"

namespace {
    using ULL = unsigned long long;
}

uint32_t DeterministicRandGenerator::generate() {
    auto ret = seed;
    seed = (static_cast<ULL>(seed) * 279410273) % 4294967291;
    return ret;
}
