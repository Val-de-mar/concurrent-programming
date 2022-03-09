#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>
#include <deque>

#include <optional>

namespace tp {

// Unbounded blocking multi-producers/multi-consumers queue

template <class Q>
struct PopFrontWhenDie {
  Q& object_;
  explicit PopFrontWhenDie(Q& object) : object_(object) {
  }
  ~PopFrontWhenDie() {
    object_.pop_front();
  }
};

template <typename T>
class UnboundedBlockingQueue {
  using Mutex = twist::stdlike::mutex;
  using CondVar = twist::stdlike::condition_variable;

 public:
  bool Put(T value) {
    std::unique_lock lock(mutex_);
    if (closed_) {
      return false;
    }
    queue_.push_back(std::move(value));
    queue_access_.notify_one();
    return true;
  }

  std::optional<T> Take() {
    std::unique_lock lock(mutex_);
    while ((!closed_) && queue_.empty()) {
      queue_access_.wait(lock);
    }
    if (closed_ && queue_.empty()) {
      return std::nullopt;
    }
    PopFrontWhenDie pop(queue_);
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
    std::unique_lock lock(mutex_);
    closed_ = true;
    if (clear) {
      queue_.clear();
    }
    queue_access_.notify_all();
  }

 private:
  bool closed_ = false;
  CondVar queue_access_;
  Mutex mutex_;
  std::deque<T> queue_;
};

}  // namespace tp
