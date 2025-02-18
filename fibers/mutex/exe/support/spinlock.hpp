#pragma once

#include <twist/stdlike/atomic.hpp>
#include <exe/fibers/core/fiber.hpp>
#include <twist/thread/spin_wait.hpp>

namespace exe::support {

// Test-and-TAS spinlock

class SpinLock {
 public:
  void Lock() {
    twist::thread::SpinWait waiter;
    while (!free_.exchange(false, std::memory_order_acquire)) {
      while (!free_.load(std::memory_order_relaxed)) {
        //        fibers::Fiber::Self().Yield();
        waiter.Spin();
      }
    }
  }

  void Unlock() {
    free_.store(true, std::memory_order_release);
  }

  // BasicLockable

  void lock() {  // NOLINT
    Lock();
  }

  void unlock() {  // NOLINT
    Unlock();
  }

 private:
  twist::stdlike::atomic<bool> free_{true};
};

}  // namespace exe::support
