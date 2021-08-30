#include <set>

class iset {
public:
    using interval_t = std::pair<uint32_t, uint32_t>;
    using iterator = std::set<interval_t>::iterator;

private:
    std::set<interval_t> _set;

public:
    bool insert(uint32_t);
    uint32_t smallest_not_included() const;
    void print() const;
};
