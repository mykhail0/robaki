#ifndef ISET_H
#define ISET_H

#include <set>

/*
Interval set data structure.
Stores intervals, merging them with newly added ones.
*/
class iset {
public:
    using interval_t = std::pair<uint32_t, uint32_t>;

private:
    std::set<interval_t> _set;

public:
    // Inserts a point into the set.
    // Same as inserting an interval like [x, x].
    // Returns true iff adding a point changed the set.
    bool insert(uint32_t);
    // Returns the smallest integer not included in the set's intervals.
    uint32_t smallest_not_included() const;
    // Prints the state of the set.
    void print() const;
};

#endif /* ISET_H */
