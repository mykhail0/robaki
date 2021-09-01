#include "../server/generator.h"
#include <iostream>

int main() {
    DeterministicRandGenerator x(1337);
    int a = 5;
    while (a--)
        std::cout << x.generate() << ' ';
    std::cout << std::endl;
    return 0;
}
