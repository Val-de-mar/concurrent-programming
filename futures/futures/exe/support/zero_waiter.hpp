#pragma once

#include <twist/stdlike/thread.hpp>
#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>
//#define CLION_DEBUG

namespace exe::support {
#ifdef CLION_DEBUG
class ZeroWaiter {
 public:
  ZeroWaiter& IncrementValue() {
    std::unique_lock lock(mutex_);
    ++counter_;
    return *this;
  };
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

 private:
  uint32_t counter_ = 0;
  twist::stdlike::mutex mutex_;
  twist::stdlike::condition_variable is_null_;
};
#else
class ZeroWaiter {
 public:
  ZeroWaiter& IncrementValue();
  ZeroWaiter& DecrementValue();
  void Wait();

 private:
  uint32_t counter_ = 0;
  twist::stdlike::mutex mutex_;
  twist::stdlike::condition_variable is_null_;
};
#endif
}  // namespace exe::support