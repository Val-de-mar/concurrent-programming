#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>
#include "iostream"

// std::lock_guard, std::unique_lock
#include <mutex>
#include <cstdint>
#include <cassert>

namespace solutions {

// CyclicBarrier allows a set of threads to all wait for each other
// to reach a common barrier point

// The barrier is called cyclic because
// it can be re-used after the waiting threads are released.

class CyclicBarrier {
 public:
  using Mutex = twist::stdlike::mutex;
  using CondVar = twist::stdlike::condition_variable;

  explicit CyclicBarrier(size_t participants)
      : participants_num_(participants), waiting_(0), first_filter_(false) {
  }

  // Blocks until all participants have invoked Arrive()
  void Arrive() {
    std::unique_lock<Mutex> lock(mutex_);
    while (first_filter_) {
      locker_.wait(lock);
    }

    ++waiting_;
    if (waiting_ == participants_num_) {
      first_filter_ = true;
      locker_.notify_all();
      --waiting_;
    } else {
      while (!first_filter_) {
        locker_.wait(lock);
      }
      --waiting_;
    }
    if (waiting_ == 0) {
      locker_.notify_all();
      first_filter_ = false;
    }
  }

 private:
  CondVar locker_;
  Mutex mutex_;
  size_t participants_num_;
  size_t waiting_;
  bool first_filter_;
};

}  // namespace solutions
