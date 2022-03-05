#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>
#include <queue>

#include <optional>

namespace tp {

// Unbounded blocking multi-producers/multi-consumers queue

template <typename T>
class UnboundedBlockingQueue {
  using Mutex = twist::stdlike::mutex;
  using CondVar = twist::stdlike::condition_variable;
  template <class Q>
  struct PopWhenDie {
    Q& object_;
    explicit PopWhenDie(Q& object) : object_(object) {
    }
    ~PopWhenDie() {
      object_.pop();
    }
  };

 public:
  bool Put(T value) {
    std::unique_lock<Mutex> lock(mutex_);
    if (closed_) {
      return false;
    }
    queue_.push(std::move(value));
    queue_access_.notify_one();
    return true;
  }

  std::optional<T> Take() {
    std::unique_lock<Mutex> lock(mutex_);
    while ((!closed_) && queue_.empty()) {
      queue_access_.wait(lock);
    }
    if (closed_ && queue_.empty()) {
      return std::nullopt;
    }
    PopWhenDie<std::queue<T>> pop(queue_);
    return {std::move(queue_.front())};
  }

  void Close() {
    CloseImpl(/*clear=*/false);
  }

  void Cancel() {
    CloseImpl(/*clear=*/true);
  }

 private:
  void CloseImpl(bool clear) {
    std::unique_lock<Mutex> lock(mutex_);
    closed_ = true;
    if (clear) {
      while (!queue_.empty()) {
        queue_.pop();
      }
    }
    queue_access_.notify_all();
  }

 private:
  bool closed_ = false;
  CondVar queue_access_;
  Mutex mutex_;
  std::queue<T> queue_;
};

}  // namespace tp
