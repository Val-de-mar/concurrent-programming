#pragma once

#include <exe/fibers/sync/futex.hpp>

#include <twist/stdlike/atomic.hpp>

namespace exe::fibers {

class Mutex {
 public:
  void Lock() {
    while (access_.exchange(true)) {
      waiter_.ParkIfEqual(true);
    }
  }

  void Unlock() {
    access_.store(false);
    waiter_.WakeOne();
  }

  // BasicLockable

  void lock() {  // NOLINT
    Lock();
  }

  void unlock() {  // NOLINT
    Unlock();
  }

 private:
  twist::stdlike::atomic<bool> access_{false};
  FutexLike<bool> waiter_{access_};
};

}  // namespace exe::fibers
