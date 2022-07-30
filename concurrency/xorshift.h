#ifndef MONOLITH_MONOLITH_NATIVE_TRAINING_RUNTIME_CONCURRENCY_XORSHIFT_H_
#define MONOLITH_MONOLITH_NATIVE_TRAINING_RUNTIME_CONCURRENCY_XORSHIFT_H_

#include <cstdint>
#include <ctime>
#include <iostream>
#include <limits>
#include <vector>

namespace monolith {
namespace concurrency {

class XorShift {
 public:
  XorShift() : p(0) {
    srand(time(0));
    x = (uint64_t)std::rand() * RAND_MAX + std::rand();
    for (uint64_t& i : s) {
      i = XorShift64Star();
    }
  }

  uint32_t Rand32() { return (uint32_t)XorShift128Plus(); }
  static uint32_t Rand32ThreadSafe();

 private:
  uint64_t XorShift1024Star();
  uint64_t XorShift128Plus();
  uint64_t XorShift64Star();

 private:
  uint64_t s[16];
  int p;
  uint64_t x; /* The state must be seeded with a nonzero value. */
};

}  // namespace concurrency
}  // namespace monolith

#endif  // MONOLITH_MONOLITH_NATIVE_TRAINING_RUNTIME_CONCURRENCY_XORSHIFT_H_
