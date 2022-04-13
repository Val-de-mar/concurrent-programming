#pragma once

#include <exe/fibers/sync/futex.hpp>
#include <exe/fibers/sync/mutex.hpp>

#include <twist/stdlike/atomic.hpp>

#include <wheels/support/defer.hpp>

#include <mutex>

namespace exe::fibers {

// https://gobyexample.com/waitgroups

class WaitGroup {
 public:
  void Add(size_t count) {
    using_.fetch_add(1);
    wheels::Defer clean([this]() {
      using_.fetch_sub(1);
    });
    auto prev = tasks_.load();
    if (prev == 0 && prev + count != 0) {
      is_done_.store(false);
    }
    tasks_.fetch_add(count);
  }

  void Done() {
    using_.fetch_add(1);
    wheels::Defer clean([this]() {
      using_.fetch_sub(1);
    });
    assert(tasks_.load() > 0);
    if (tasks_.fetch_sub(1) == 1) {
      is_done_.store(true);
      done_wait_.WakeAll();
    }
  }

  void Wait() {
    while (!is_done_) {
      done_wait_.ParkIfEqual(false);
    }
    twist::thread::SpinWait wait;
    while (using_ != 0) {
      wait.Spin();
    }
  }

 private:
  twist::stdlike::atomic<int64_t> tasks_{0};
  twist::stdlike::atomic<bool> is_done_{false};
  twist::stdlike::atomic<uint64_t> using_{0};
  FutexLike<bool> done_wait_{is_done_};
};

}  // namespace exe::fibers
