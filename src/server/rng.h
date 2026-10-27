#ifndef RNG_H
#define RNG_H

#include <cstdint>

namespace server {
class RNG {
 protected:
  uint32_t seed;

 public:
  explicit RNG(uint32_t seed) : seed(seed) {}
  virtual uint32_t generate() = 0;
};

class DeterministicRNG : public RNG {
 public:
  explicit DeterministicRNG(uint32_t seed) : RNG(seed) {}
  // r_0 = seed
  // r_i = (r_{i-1} * 279410273) mod 4294967291
  uint32_t generate();
};
}  // namespace server

#endif /* RNG_H */
