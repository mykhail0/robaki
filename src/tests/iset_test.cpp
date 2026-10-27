#include "../client/iset.h"

#include <cassert>
#include <vector>

int main() {
  client::iset s;

  std::vector<uint32_t> test{2, 3, 7, 6, 4, 8, 5, 1, 0};
  std::vector<std::vector<client::iset::interval_t>> snapshots(test.size());

  // [2]
  snapshots[0].emplace_back(2, 2);

  // [2, 3]
  snapshots[1].emplace_back(2, 3);

  // [2, 3], [7]
  snapshots[2].emplace_back(2, 3);
  snapshots[2].emplace_back(7, 7);

  // [2, 3], [6, 7]
  snapshots[3].emplace_back(2, 3);
  snapshots[3].emplace_back(6, 7);

  // [2, 3, 4], [6, 7]
  snapshots[4].emplace_back(2, 4);
  snapshots[4].emplace_back(6, 7);

  // [2, 3, 4], [6, 7, 8]
  snapshots[5].emplace_back(2, 4);
  snapshots[5].emplace_back(6, 8);

  // [2, 3, 4, 5, 6, 7, 8]
  snapshots[6].emplace_back(2, 8);

  // [1, 2, 3, 4, 5, 6, 7, 8]
  snapshots[7].emplace_back(1, 8);

  // [0, 1, 2, 3, 4, 5, 6, 7, 8]
  snapshots[8].emplace_back(0, 8);

  for (size_t i = 0; i < test.size(); ++i) {
    s.insert(test[i]);
    size_t j = 0;
    for (auto it = s.cbegin(); it != s.cend(); ++it) {
      assert(snapshots[i][j].first == it->first);
      assert(snapshots[i][j].second == it->second);
      ++j;
    }
  }

  return 0;
}
