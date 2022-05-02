#pragma once

#include <exe/fibers/sync/mutex.hpp>
#include <exe/fibers/sync/futex.hpp>

#include <twist/stdlike/atomic.hpp>

// std::unique_lock
#include <mutex>

namespace exe::fibers {

class CondVar {
  using Lock = std::unique_lock<Mutex>;

 public:
  void Wait(Lock& lock) {
    auto noticed = val_.load();
    lock.unlock();
    bell_.ParkIfEqual(noticed);
    lock.lock();
  }

  void NotifyOne() {
    val_.fetch_add(1);
    bell_.WakeOne();
  }

  void NotifyAll() {
    val_.fetch_add(1);
    bell_.WakeAll();
  }

 private:
  twist::stdlike::atomic<uint64_t> val_{0};
  FutexLike<uint64_t> bell_{val_};
};

}  // namespace exe::fibers
