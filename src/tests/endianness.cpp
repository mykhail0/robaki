#include "../utility.h"
#include <iostream>

int main() {
    byte_t a = 4;
    auto i = a;
    auto A = BEbytes2num<byte_t>(&i);
    std::cout << "Should be: " << a << ", is: " << A << ", big endian: " << i << std::endl;

    uint32_t b = 4;
    auto j = htonl(b);
    auto B = BEbytes2num<uint32_t>((byte_t *) &j);
    std::cout << "Should be: " << b << ", is: " << B << ", htonl: " << j << std::endl;

    uint64_t c = 4;
    auto k = htobe64(c);
    auto C = BEbytes2num<uint64_t>((byte_t *) &k);
    std::cout << "Should be: " << c << ", is: " << C << ", htobe64: " << k << std::endl;

    return 0;
}
