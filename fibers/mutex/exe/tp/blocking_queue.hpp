#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>

#include <wheels/support/defer.hpp>
#include <wheels/intrusive/forward_list.hpp>

#include <deque>
#include <optional>

namespace exe::detail {

// Unbounded blocking multi-producers/multi-consumers queue

template <typename T>
class UnboundedIntrusiveBlockingQueue {
  using Node = wheels::IntrusiveForwardListNode<T>;

 public:
  bool Put(T* value) {
    std::lock_guard lock(mutex_);
    if (closed_) {
      return false;
    }
    queue_.PushBack(static_cast<Node*>(value));
    queue_access_.notify_one();
    return true;
  }

  std::optional<T*> Take() {
    std::unique_lock lock(mutex_);
    while ((!closed_) && queue_.IsEmpty()) {
      queue_access_.wait(lock);
    }
    if (closed_ && queue_.IsEmpty()) {
      return std::nullopt;
    }

    return queue_.PopFront();
  }

  void Close() {
    std::unique_lock lock(mutex_);
    closed_ = true;
    queue_access_.notify_all();
  }

  template <typename FunctionT>
  void Cancel(FunctionT func) {
    std::unique_lock lock(mutex_);
    closed_ = true;
    while (!queue_.IsEmpty()) {
      func(queue_.PopFront());
    }
    queue_access_.notify_all();
  }

 private:
  bool closed_ = false;
  twist::stdlike::condition_variable queue_access_;
  twist::stdlike::mutex mutex_;
  wheels::IntrusiveForwardList<T> queue_;
};

}  // namespace exe::detail