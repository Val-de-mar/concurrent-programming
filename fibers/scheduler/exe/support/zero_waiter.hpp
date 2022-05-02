
#pragma once

#include <twist/stdlike/thread.hpp>
#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>

#include <cassert>

namespace exe::support {
class ZeroWaiter {
 public:
  explicit ZeroWaiter(size_t init_value = 0) : counter_(init_value) {
  }

  ZeroWaiter& IncrementValue() {
    std::unique_lock lock(mutex_);
    ++counter_;
    return *this;
  }
  ZeroWaiter& DecrementValue() {
    std::unique_lock lock(mutex_);
    assert(counter_ != 0);
    --counter_;
    if (counter_ == 0) {
      is_null_.notify_all();
    }
    return *this;
  }
  void Wait() {
    std::unique_lock lock(mutex_);
    while (counter_ != 0) {
      is_null_.wait(lock);
    }
  }
  uint32_t GetVal() {
    return counter_;
  }

 private:
  uint32_t counter_;
  twist::stdlike::mutex mutex_;
  twist::stdlike::condition_variable is_null_;
};

}  // namespace exe::support
