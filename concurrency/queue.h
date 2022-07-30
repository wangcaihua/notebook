#ifndef CONCURRENCY_QUEUE_H_
#define CONCURRENCY_QUEUE_H_

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

namespace std {
namespace concurrency {

template <typename T, bool Ordered = false>
class Queue {
 public:
  // Create a queue object with a given maximum size(default: max_size=1).
  // If max_size is 0, the queue size is infinite.
  explicit Queue(size_t max_size = 1)
      : max_size_(max_size == 0 ? std::numeric_limits<size_t>::max()
                                : max_size) {}

  Queue(const Queue&) = delete;  // disable copying

  Queue& operator=(const Queue&) = delete;  // disable assignment

  // Return the front item of the queue, it blocks if no item was available.
  T front() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty()) {
      enqueue_cond_.wait(lock);
    }
    return _top();
  }

  // Remove and return an item from the queue, it blocks if no item was
  // available.
  T pop() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty()) {
      enqueue_cond_.wait(lock);
    }
    auto val = _top();
    queue_.pop();
    lock.unlock();
    dequeue_cond_.notify_one();
    return val;
  }

  // Remove an item(and assign to T& item) from the queue, it blocks if
  // no item was available.
  void pop(T& item) {  // NOLINT
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.empty()) {
      enqueue_cond_.wait(lock);
    }
    item = _top();
    queue_.pop();
    lock.unlock();
    dequeue_cond_.notify_one();
  }

  // Try to remove an item(and assign to T& item) from the queue, it blocks
  // at most 'timeout' duration and return false if no item was available
  // within that time.
  template <typename Rep = int64_t, typename Period = std::milli>
  bool try_pop(T& item, std::chrono::duration<Rep, Period> timeout) {  // NOLINT
    std::unique_lock<std::mutex> lock(mutex_);
    if (!enqueue_cond_.wait_for(lock, timeout,
                                [this] { return !queue_.empty(); })) {
      return false;
    }

    item = _top();
    queue_.pop();
    lock.unlock();
    dequeue_cond_.notify_one();
    return true;
  }

  // Try to push an item into the queue, it blocks at most 'timeout'
  // duration and return false if no free slot was available within
  // that time.
  template <typename Rep = int64_t, typename Period = std::milli>
  bool try_push(T item, std::chrono::duration<Rep, Period> timeout) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!dequeue_cond_.wait_for(lock, timeout,
                                [this] { return queue_.size() < max_size_; })) {
      return false;
    }

    queue_.push(std::move(item));
    lock.unlock();
    enqueue_cond_.notify_one();
    return true;
  }

  // Put an item into the queue, it blocks if no free slot was available.
  void push(T item) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.size() >= max_size_) {
      dequeue_cond_.wait(lock);
    }

    queue_.push(std::move(item));
    lock.unlock();
    enqueue_cond_.notify_one();
  }

  // Return true if the queue is empty, false otherwise (not reliable).
  bool empty() {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.empty();
  }

 private:
  template <bool enabled = Ordered>
  inline typename std::enable_if<enabled, T>::type _top() {
    return queue_.top();
  }

  template <bool enabled = Ordered>
  inline typename std::enable_if<!enabled, T>::type _top() {
    return queue_.front();
  }

 private:
  size_t max_size_;

  typename std::conditional<Ordered, std::priority_queue<T>,
                            std::queue<T>>::type queue_;

  std::mutex mutex_;

  std::condition_variable enqueue_cond_;

  std::condition_variable dequeue_cond_;
};

}  // namespace concurrency
}  // namespace std

#endif  // CONCURRENCY_QUEUE_H_
