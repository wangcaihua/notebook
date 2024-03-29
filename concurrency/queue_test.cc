#include "concurrency/queue.h"

#include <atomic>
#include <memory>
#include <thread>

#include "gtest/gtest.h"

using std::chrono::duration_cast;
using std::chrono::high_resolution_clock;
using std::chrono::microseconds;
using std::chrono::milliseconds;

namespace std {
namespace concurrency {

float PushTimeout(int timeout /* milliseconds */) {
  Queue<int> queue(1);
  queue.push(1);
  auto start = high_resolution_clock::now();
  EXPECT_FALSE(queue.try_push(2, milliseconds(timeout)));
  auto elapsed = high_resolution_clock::now() - start;

  return duration_cast<microseconds>(elapsed).count() / 1000.f;
}

float PopTimeout(int timeout /* milliseconds */) {
  Queue<int> queue(1);
  queue.push(1);
  int item = queue.pop();
  EXPECT_EQ(item, 1);

  auto start = high_resolution_clock::now();
  EXPECT_FALSE(queue.try_pop(item, milliseconds(timeout)));
  auto elapsed = high_resolution_clock::now() - start;

  return duration_cast<microseconds>(elapsed).count() / 1000.f;
}

TEST(QueueTest, Basic) {
  std::atomic_int producer_count(0);
  std::atomic_int consumer_count(0);
  std::atomic<bool> done(false);
  Queue<int> queue(128);

  const int iterations = 10 * 10000;
  const int producer_thread_count = 10;
  const int consumer_thread_count = 10;
  const std::chrono::microseconds timeout(10);

  auto producer = [&]() {
    for (int i = 0; i != iterations; ++i) {
      int value = ++producer_count;
      while (!queue.try_push(value, timeout)) {}
    }
  };

  auto consumer = [&]() {
    int value;
    while (!done) {
      while (queue.try_pop(value, timeout))
        ++consumer_count;
    }

    while (queue.try_pop(value, timeout))
      ++consumer_count;
  };

  std::vector<std::thread> producer_threads, consumer_threads;

  for (int i = 0; i != producer_thread_count; ++i) {
    producer_threads.emplace_back(producer);
  }
  for (int i = 0; i != consumer_thread_count; ++i) {
    consumer_threads.emplace_back(consumer);
  }

  for (auto& t : producer_threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  done = true;
  for (auto& t : consumer_threads) {
    if (t.joinable()) {
      t.join();
    }
  }

  EXPECT_EQ(producer_count, iterations * producer_thread_count);
  EXPECT_EQ(consumer_count, iterations * consumer_thread_count);
}

TEST(QueueTest, Timeout) {
  EXPECT_NEAR(PushTimeout(1), 1.f, 0.5);
  EXPECT_NEAR(PushTimeout(10), 10.f, 2);
  EXPECT_NEAR(PushTimeout(1000), 1000.f, 20);

  EXPECT_NEAR(PopTimeout(1), 1.f, 0.5);
  EXPECT_NEAR(PopTimeout(10), 10.f, 2);
  EXPECT_NEAR(PopTimeout(1000), 1000.f, 20);
}

}  // namespace concurrency
}  // namespace std
