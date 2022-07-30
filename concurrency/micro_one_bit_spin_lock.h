#ifndef CONCURRENCY_MICRO_ONE_BIT_SPIN_LOCK_H_
#define CONCURRENCY_MICRO_ONE_BIT_SPIN_LOCK_H_

#include "concurrency/sleeper.h"
#include "glog/logging.h"


namespace std {
namespace concurrency {

// ensure never modify the other bits of lock_ when are not holding the lock
struct MicroOneBitSpinLock {
 private:
  static const uint8_t MASK = 0x1;

 public:
  enum { FREE = 0, LOCKED = MASK };
  // lock_ can't be std::atomic<> to preserve POD-ness.
  mutable uint8_t lock_;

  // Initialize this MSL.  It is unnecessary to call this if you
  // zero-initialize the MicroSpinLock.
  void Init() {
    Payload()->store(Payload()->load() & ~MASK);
  }

  bool TryLock() {
    uint8_t val = Payload()->load();
    return CompareAndSwap(val & ~MASK, val | MASK);
  }

  void Lock() {
    Sleeper sleeper;
    do {
      while ((Payload()->load() & MASK) != FREE) {
        sleeper.Wait();
      }
    } while (!TryLock());
    DCHECK((Payload()->load() & MASK) == LOCKED);
  }

  void Unlock() {
    uint8_t val = Payload()->load();
    CHECK((val & MASK) == LOCKED);
    Payload()->store(val & ~MASK, std::memory_order_release);
  }

  uint8_t Value() const {
    return Payload()->load() >> 1;
  }

  void Set(uint8_t val) {
    Payload()->store((val << 1) + (Payload()->load() & MASK));
  }

 private:
  std::atomic<uint8_t>* Payload() const {
    return reinterpret_cast<std::atomic<uint8_t>*>(&this->lock_);
  }

  bool CompareAndSwap(uint8_t compare, uint8_t newVal) {
    return std::atomic_compare_exchange_strong_explicit(Payload(), &compare, newVal,
                                                        std::memory_order_acquire,
                                                        std::memory_order_relaxed);
  }
};

}  // namespace concurrency
}  // namespace std

#endif  // CONCURRENCY_MICRO_ONE_BIT_SPIN_LOCK_H_
