#include <iostream>

#include "../utility.h"

int main() {
  byte_t a = 4;
  byte_t i = a;
  byte_t A = BEbytes_to_num<byte_t>(&i);
  if (a != A) {
    std::cerr << "Should be: " << a << ", is: " << A << ", big endian: " << i
              << std::endl;
    return EXIT_FAILURE;
  }

  uint32_t b = 4;
  uint32_t j = htonl(b);
  uint32_t B = BEbytes_to_num<uint32_t>((byte_t*)&j);
  if (b != B) {
    std::cerr << "Should be: " << b << ", is: " << B << ", htonl: " << j
              << std::endl;
    return EXIT_FAILURE;
  }

  uint64_t c = 4;
  uint64_t k = htobe64(c);
  uint64_t C = BEbytes_to_num<uint64_t>((byte_t*)&k);
  if (c != C) {
    std::cerr << "Should be: " << c << ", is: " << C << ", htobe64: " << k
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
