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
      : participants_num_(participants), waiting_(0), ready_to_go_(false) {
  }

  // Blocks until all participants have invoked Arrive()
  void Arrive() {
    /*
    { std::lock_guard enter(enter_); }
    std::unique_lock lock(mutex_);
    ready_to_go_ = false;
    ++waiting_;
    if (participants_num_ == waiting_) {
      lock.unlock();
      std::lock_guard enter(enter_);
      ready_to_go_ = true;
      lock.lock();
      while (waiting_ != 1) {
        lock.unlock();
        locker_.notify_all();
        lock.lock();
      }
      lock.unlock();
      --waiting_;
    } else {
      while (!ready_to_go_) {
        locker_.wait(lock);
      }
      --waiting_;
    }*/

    std::unique_lock<Mutex> lock(mutex_);
    while (ready_to_go_) {
      locker_.wait(lock);
    }

    ++waiting_;
    if (waiting_ == participants_num_) {
      ready_to_go_ = true;
      locker_.notify_all();
      --waiting_;
    } else {
      while (!ready_to_go_) {
        locker_.wait(lock);
      }
      --waiting_;
    }
    if (waiting_ == 0) {
      locker_.notify_all();
      ready_to_go_ = false;
    }
  }

 private:
  CondVar locker_;
  Mutex mutex_;
  Mutex enter_;
  Mutex checker_;
  size_t participants_num_;
  size_t waiting_;
  bool ready_to_go_;
};

}  // namespace solutions
