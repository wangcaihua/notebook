#include "concurrency/utils.h"
#include "glog/logging.h"

namespace std {
namespace concurrency {

// detection for 64 bit
#if defined(__x86_64__) || defined(_M_X64)
# define FOLLY_X64 1
#else
# define FOLLY_X64 0
#endif

#if defined(__aarch64__)
# define FOLLY_AARCH64 1
#else
# define FOLLY_AARCH64 0
#endif

inline void asm_volatile_pause() {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
  ::_mm_pause();
#elif defined(__i386__) || FOLLY_X64
  asm volatile("pause");
#elif FOLLY_AARCH64 || defined(__arm__)
  asm volatile("yield");
#elif FOLLY_PPC64
  asm volatile("or 27,27,27");
#endif
}

class Sleeper {
 public:
  Sleeper() : spinCount(0) {}

  void Wait() {
    if (spinCount < kMaxActiveSpin) {
      ++spinCount;
      asm_volatile_pause();
    } else {
      struct timespec ts = {0, 500000};
      nanosleep(&ts, nullptr);
    }
  }

 private:
  static const uint32_t kMaxActiveSpin = 4000;
  uint32_t spinCount;
};

void MicroOneBitSpinLock::Init() {
  Payload()->store(Payload()->load() & ~MASK);
}

bool MicroOneBitSpinLock::TryLock() {
  uint8_t val = Payload()->load();
  return CompareAndSwap(val & ~MASK, val | MASK);
}

void MicroOneBitSpinLock::Lock() {
  Sleeper sleeper;
  do {
    while ((Payload()->load() & MASK) != FREE) {
      sleeper.Wait();
    }
  } while (!TryLock());
  DCHECK((Payload()->load() & MASK) == LOCKED);
}

void MicroOneBitSpinLock::Unlock() {
  uint8_t val = Payload()->load();
  CHECK((val & MASK) == LOCKED);
  Payload()->store(val & ~MASK, std::memory_order_release);
}

uint8_t MicroOneBitSpinLock::Value() const {
  return Payload()->load() >> 1;
}

void MicroOneBitSpinLock::Set(uint8_t val) {
  Payload()->store((val << 1) + (Payload()->load() & MASK));
}

std::atomic<uint8_t>* MicroOneBitSpinLock::Payload() const {
  return reinterpret_cast<std::atomic<uint8_t>*>(&this->lock_);
}

bool MicroOneBitSpinLock::CompareAndSwap(uint8_t compare, uint8_t newVal) {
  return std::atomic_compare_exchange_strong_explicit(Payload(), &compare, newVal,
                                                      std::memory_order_acquire,
                                                      std::memory_order_relaxed);
}


XorShift::XorShift() : p(0) {
  srand(time(0));
  x = (uint64_t)std::rand() * RAND_MAX + std::rand();
  for (uint64_t& i : s) {
    i = XorShift64Star();
  }
}

uint64_t XorShift::XorShift1024Star() {
  uint64_t s0 = s[p];
  uint64_t s1 = s[p = (p + 1) & 15];
  s1 ^= s1 << 31;  // a
  s1 ^= s1 >> 11;  // b
  s0 ^= s0 >> 30;  // c
  return (s[p] = s0 ^ s1) * UINT64_C(1181783497276652981);
}

uint64_t XorShift::XorShift128Plus() {
  uint64_t x = s[0];
  uint64_t const y = s[1];
  s[0] = y;
  x ^= x << 23;        // a
  x ^= x >> 17;        // b
  x ^= y ^ (y >> 26);  // c
  s[1] = x;
  return x + y;
}

uint64_t XorShift::XorShift64Star() {
  x ^= x >> 12;  // a
  x ^= x << 25;  // b
  x ^= x >> 27;  // c
  return x * UINT64_C(2685821657736338717);
}

uint32_t XorShift::Rand32ThreadSafe() {
  static thread_local XorShift xor_shift;
  return xor_shift.Rand32();
}


SimpleThreadPool::SimpleThreadPool(int num_threads) {
  for (int i = 0; i < num_threads; ++i) {
    threads_.emplace_back(&SimpleThreadPool::WorkLoop, this);
  }
}

SimpleThreadPool::~SimpleThreadPool() {
  {
    absl::MutexLock l(&mu_);
    for (size_t i = 0; i < threads_.size(); i++) {
      queue_.push(nullptr);  // Shutdown signal.
    }
  }
  for (auto &t : threads_) {
    t.join();
  }
}

void SimpleThreadPool::Schedule(std::function<void()> func) {
  assert(func != nullptr);
  absl::MutexLock l(&mu_);
  queue_.push(std::move(func));
}

void SimpleThreadPool::WorkLoop() {
  while (true) {
    std::function<void()> func;
    {
      absl::MutexLock l(&mu_);
      mu_.Await(absl::Condition(this, &SimpleThreadPool::WorkAvailable));
      func = std::move(queue_.front());
      queue_.pop();
    }
    if (func == nullptr) {  // Shutdown signal.
      break;
    }
    func();
  }
}

}  // namespace concurrency
}  // namespace std
