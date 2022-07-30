#ifndef HOPSCOTCH_HOPSCOTCH_HASH_SET_H_
#define HOPSCOTCH_HOPSCOTCH_HASH_SET_H_

#include <atomic>
#include <thread>
#include <unordered_set>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/hash/hash.h"
#include "concurrency/utils.h"
#include "hopscotch/murmurhash.h"

namespace std {
namespace hopscotch {

using FID = int64_t;

#pragma pack(push)
#pragma pack(4)

template <typename Key>
struct hopscotch_entry_t {
  Key key;
  uint32_t hash;
  uint32_t hop_info;
};
#pragma pack(pop)

template <class Key>
Key GetEmptyValue() {
  return Key();
}

template <>
FID GetEmptyValue<FID>() {
  return -1;
}

template <>
std::pair<int64_t, const void*>
GetEmptyValue<std::pair<int64_t, const void*>>() {
  return std::make_pair(-1, nullptr);
}

// thread safe hopscotch hash set (insert only)
// paper:
// http://people.csail.mit.edu/shanir/publications/disc2008_submission_98.pdf
template <typename Key>
class HopscotchHashSet {
 public:
  explicit HopscotchHashSet(uint32_t capacity, uint32_t concurrency_level);

  // thread safe insert, return number keys cleared
  size_t insert(Key key);

  std::vector<Key> GetAndClear();

  size_t size() const { return num_elements_.load(std::memory_order_relaxed); }

  uint32_t capacity() const { return capacity_; }

 private:
  uint32_t HashFunc(Key key) { return hash_func_(key) | 3; }

  void FindCloserFreeBucket(const concurrency::MicroOneBitSpinLock* lock,
                            int* free_bucket, int* free_dist);

  void DoInit();

  // clear the hash table, not thread safe
  void DoClear();

 private:
  static constexpr uint32_t kHopscotchHashInsertRange = 4096;
  static constexpr uint32_t kHopscotchHashHopRange = 32;
  static constexpr uint32_t kHopscotchHashEmpty = 0;
  static constexpr uint32_t kHopscotchHashBusy = 1;
  Key kEmptyKey = GetEmptyValue<Key>();

  absl::Hash<Key> hash_func_;

  // for those keys not insert into table
  absl::flat_hash_set<Key> extra_;
  concurrency::MicroOneBitSpinLock extra_lock_;
  concurrency::MicroOneBitSpinLock clear_lock_;
  concurrency::MicroOneBitSpinLock init_lock_;
  std::vector<hopscotch_entry_t<Key>> table_;
  std::vector<concurrency::MicroOneBitSpinLock> locks_;
  uint32_t lock_mask_;
  uint32_t bucket_mask_;
  std::atomic_int running_threads_;  // number of thread doing insertion
  std::atomic_int num_elements_;     // total number of elements
  uint32_t capacity_;
  bool init_;
};

}  // namespace hopscotch
}  // namespace std

#endif  // HOPSCOTCH_HOPSCOTCH_HASH_SET_H_
