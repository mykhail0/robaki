#include <cstdint>
#include <iostream>

#include "../server/rng.h"

int main() {
  uint32_t seed;
  int i;
  std::cin >> seed >> i;
  server::DeterministicRNG x(seed);
  while (i--) {
    std::cout << x.generate() << std::endl;
  }
  return 0;
}
