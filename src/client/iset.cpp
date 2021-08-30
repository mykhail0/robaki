#include "iset.h"
#include <cassert>
#include <iostream>

bool iset::insert(uint32_t e) {
    interval_t i {e, e};

    if (_set.empty()) {
        _set.insert(i);
        return true;
    }

    auto it = _set.lower_bound(i);

    if (it == _set.end()) {
        --it;
        if (it->second < e) {
            if (it->second + 1 == e) {
                i.first = it->first;
                it = _set.erase(it);
            } else {
                it = _set.end();
            }
            _set.insert(it, i);
            return true;
        }
        assert(it->first < e && e <= it->second);
        return false;
    }

    assert(e <= it->first);

    if (e == it->first)
        return false;

    assert(e < it->first);

    if (it == _set.begin()) {
        if (e + 1 == it->first) {
            i.second = it->second;
            it = _set.erase(it);
        }
        _set.insert(it, i);
        return true;
    }

    auto bigger = it;
    --it;
    if (e <= it->second)
        return false;

    assert(it->second < e && e < bigger->first);
    if (it->second + 1 == e) {
        i.first = it->first;
        _set.erase(it);
    }

    if (e + 1 == bigger->first) {
        i.second = bigger->second;
        _set.erase(bigger);
    }

    _set.insert(i);
    return true;
}

uint32_t iset::smallest_not_included() const {
    if (_set.empty())
        return 0;
    auto it = _set.begin();
    return it->first == 0 ? it->second + 1 : 0;
}

void iset::print() const {
    for (auto &p: _set)
        std::cout << "[" << p.first << ", " << p.second << "] ";
    std::cout << "smallest not included: " << smallest_not_included() << std::endl;
}
