#ifndef CONCURRENCY_UTILS_H_
#define CONCURRENCY_UTILS_H_

#include <cstdint>
#include <ctime>
#include <iostream>
#include <limits>
#include <vector>
#include <cassert>
#include <cstddef>
#include <functional>
#include <queue>
#include <thread>

#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"


namespace std {
namespace concurrency {

struct MicroOneBitSpinLock {
 private:
  static const uint8_t MASK = 0x1;
  mutable uint8_t lock_;

 public:
  enum { FREE = 0, LOCKED = MASK };
  
  void Init(); 
  bool TryLock();
  void Lock();
  void Unlock();
  uint8_t Value() const;
  void Set(uint8_t val);

 private:
  std::atomic<uint8_t>* Payload() const;
  bool CompareAndSwap(uint8_t compare, uint8_t newVal);
};

class XorShift {
 public:
  XorShift();
  inline uint32_t Rand32() { return (uint32_t)XorShift128Plus(); }
  static uint32_t Rand32ThreadSafe();

 private:
  uint64_t XorShift1024Star();
  uint64_t XorShift128Plus();
  uint64_t XorShift64Star();

  uint64_t s[16];
  int p;
  uint64_t x;
};

class SimpleThreadPool {
 public:
  explicit SimpleThreadPool(int num_threads);
  SimpleThreadPool(const SimpleThreadPool &) = delete;
  SimpleThreadPool &operator=(const SimpleThreadPool &) = delete;
  ~SimpleThreadPool();

  void Schedule(std::function<void()> func);

 private:
  inline bool WorkAvailable() const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mu_) {
    return !queue_.empty();
  }

  void WorkLoop();

  absl::Mutex mu_;
  std::queue<std::function<void()>> queue_ ABSL_GUARDED_BY(mu_);
  std::vector<std::thread> threads_;
};

}  // namespace concurrency
}  // namespace std

#endif  // CONCURRENCY_UTILS_H_
