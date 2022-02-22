#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>

// std::lock_guard, std::unique_lock
#include <mutex>
#include <cstdint>

namespace solutions {

// A Counting semaphore

// Semaphores are often used to restrict the number of threads
// than can access some (physical or logical) resource

class Semaphore {
  using CondVar = twist::stdlike::condition_variable;
  using Mutex = twist::stdlike::mutex;

 public:
  // Creates a Semaphore with the given number of permits
  explicit Semaphore(size_t initial) : tokens_(initial) {
  }

  // Acquires a permit from this semaphore,
  // blocking until one is available
  void Acquire() {
    std::unique_lock<Mutex> token_appearance(mutex_);
    while (tokens_ == 0) {
      empty_token_set_.wait(token_appearance);
    }
    --tokens_;
  }

  // Releases a permit, returning it to the semaphore
  void Release() {
    std::lock_guard<Mutex> lock(mutex_);
    ++tokens_;
    empty_token_set_.notify_one();
  }

 private:
  // Permits
  CondVar empty_token_set_;
  Mutex mutex_;
  size_t tokens_;
};

}  // namespace solutions
