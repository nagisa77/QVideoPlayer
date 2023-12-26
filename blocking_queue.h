#ifndef BLOCKING_QUEUE_
#define BLOCKING_QUEUE_

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <queue>

template<typename T>
class BlockingQueue {
public:
    void replace(std::queue<T> queue) {
      boost::mutex::scoped_lock lock(mutex_);
      queue_ = queue;
      condition_.notify_one();
    }
  
    void push(const T& value) {
        boost::mutex::scoped_lock lock(mutex_);
        queue_.push(value);
        condition_.notify_one();
    }

    T pop() {
        boost::mutex::scoped_lock lock(mutex_);
        while (queue_.empty()) {
            condition_.wait(lock);
        }
        T value = queue_.front();
        queue_.pop();
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
    std::queue<T> queue_;
};

#endif
