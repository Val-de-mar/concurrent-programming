#pragma once

#include <twist/stdlike/atomic.hpp>

#include <cstdint>

namespace stdlike {

class CondVar {
 public:
  template <class T>
  using Atomic = twist::stdlike::atomic<T>;
  // Mutex - BasicLockable
  // https://en.cppreference.com/w/cpp/named_req/BasicLockable
  template <class Mutex>
  void Wait(Mutex& mutex) {
    auto noticed = val_.load();
    mutex.unlock();
    val_.wait(noticed);
    mutex.lock();
  }

  void NotifyOne() {
    val_.fetch_add(1);
    val_.notify_one();
  }

  void NotifyAll() {
    val_.fetch_add(1);
    val_.notify_all();
  }

 private:
  Atomic<uint32_t> val_{0};
};

}  // namespace stdlike
