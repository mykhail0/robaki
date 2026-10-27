#ifndef ISET_H
#define ISET_H

#include <cstdint>
#include <set>

namespace client {
// Interval set data structure for storing intervals and merging them with newly
// added ones. Intended for nonnegative integers.
class iset {
 public:
  using interval_t = std::pair<uint32_t, uint32_t>;
  using const_iterator = std::set<interval_t>::const_iterator;

 private:
  std::set<interval_t> _set;

 public:
  // Insert a point into the set.
  // Same as inserting an interval like [x, x].
  // Return `true` iff adding a point changed the set.
  bool insert(uint32_t);
  // Return the smallest integer not included in the set's intervals.
  uint32_t smallest_not_included() const;
  // Methods for iterating over the iset.
  const_iterator cbegin() const noexcept;
  const_iterator cend() const noexcept;
};
}  // namespace client

#endif /* ISET_H */
