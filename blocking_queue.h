#ifndef BLOCKING_QUEUE_
#define BLOCKING_QUEUE_

#include <boost/thread/condition_variable.hpp>
#include <boost/thread/mutex.hpp>
#include <queue>

template <typename T>
class BlockingQueue {
 public:
  explicit BlockingQueue(size_t max_length) : max_length_(max_length) {}

  void replace(std::queue<T> queue) {
    boost::mutex::scoped_lock lock(mutex_);
    queue_ = queue;
    condition_.notify_one();
  }

  void push(const T& value) {
    boost::mutex::scoped_lock lock(mutex_);
    while (queue_.size() >= max_length_) {
      condition_full_.wait(lock);  // 等待队列不满
    }
    queue_.push(value);
    condition_.notify_one();  // 通知等待的 pop
  }

  std::optional<T> popOrEmpty() {
    boost::mutex::scoped_lock lock(mutex_);

    if (queue_.empty()) {
      return {};
    }

    T value = queue_.front();
    queue_.pop();

    return value;
  }

  T pop() {
    boost::mutex::scoped_lock lock(mutex_);
    while (queue_.empty()) {
      condition_.wait(lock);
    }
    T value = queue_.front();
    queue_.pop();
    if (queue_.size() < max_length_) {
      condition_full_.notify_one();  // 通知等待的 push
    }
    return value;
  }

  bool empty() const {
    boost::mutex::scoped_lock lock(mutex_);
    return queue_.empty();
  }

  size_t size() const {
    boost::mutex::scoped_lock lock(mutex_);
    return queue_.size();
  }

 private:
  mutable boost::mutex mutex_;
  boost::condition_variable condition_;
  boost::condition_variable condition_full_;  // 新增
  std::queue<T> queue_;
  size_t max_length_;
};

#endif
