#include "iset.h"
#include <vector>

int main() {
    iset s;
    s.print();

    std::vector<uint32_t> test {
        2, 3, 7, 6, 4, 8, 5, 1, 0
    };

    for (int i: test) {
        s.insert(i);
        s.print();
    }

    return 0;
}
