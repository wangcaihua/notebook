//
// Created by david on 2020-12-01.
//

#ifndef MONOLITH_MONOLITH_NATIVE_TRAINING_RUNTIME_CONCURRENCY_THREAD_POOL_H_
#define MONOLITH_MONOLITH_NATIVE_TRAINING_RUNTIME_CONCURRENCY_THREAD_POOL_H_

#include <cassert>
#include <cstddef>
#include <functional>
#include <queue>
#include <thread>
#include <vector>
#include "absl/base/thread_annotations.h"
#include "absl/synchronization/mutex.h"

namespace monolith {
namespace concurrency {
/**
 * A simple ThreadPool implementation for tests/benchmarks.
 */
class ThreadPool {
 public:
  explicit ThreadPool(int num_threads) {
    for (int i = 0; i < num_threads; ++i) {
      threads_.emplace_back(&ThreadPool::WorkLoop, this);
    }
  }

  ThreadPool(const ThreadPool &) = delete;
  ThreadPool &operator=(const ThreadPool &) = delete;

  ~ThreadPool();

  // Schedule a function to be run on a ThreadPool thread immediately.
  void Schedule(std::function<void()> func);

 private:
  bool WorkAvailable() const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mu_) {
    return !queue_.empty();
  }

  void WorkLoop();

  absl::Mutex mu_;
  std::queue<std::function<void()>> queue_ ABSL_GUARDED_BY(mu_);
  std::vector<std::thread> threads_;
};

}  // namespace concurrency
}  // namespace monolith

#endif  // MONOLITH_MONOLITH_NATIVE_TRAINING_RUNTIME_CONCURRENCY_THREAD_POOL_H_
