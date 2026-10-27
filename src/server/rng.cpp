#include "rng.h"

uint32_t server::DeterministicRNG::generate() {
  uint32_t ret = seed;
  seed = (static_cast<unsigned long long>(seed) * 279410273) % 4294967291;
  return ret;
}
