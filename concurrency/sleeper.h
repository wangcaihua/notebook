#ifndef CONCURRENCY_SLEEPER_H_
#define CONCURRENCY_SLEEPER_H_

namespace monolith {
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

/*
 * A helper object for the contended case. Starts off with eager
 * spinning, and falls back to sleeping for small quantums.
 */
class Sleeper {
  static const uint32_t kMaxActiveSpin = 4000;

  uint32_t spinCount;

 public:
  Sleeper() : spinCount(0) {}

  void Wait() {
    if (spinCount < kMaxActiveSpin) {
      ++spinCount;
      asm_volatile_pause();
    } else {
      /*
       * Always sleep 0.5ms, assuming this will make the kernel put
       * us down for whatever its minimum timer resolution is (in
       * linux this varies by kernel version from 1ms to 10ms).
       */
      struct timespec ts = {0, 500000};
      nanosleep(&ts, nullptr);
    }
  }
};

}  // namespace concurrency
}  // namespace monolith

#endif  // CONCURRENCY_SLEEPER_H_
