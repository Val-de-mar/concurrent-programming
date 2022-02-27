#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>

#include <memory>
#include <cassert>
#include <variant>
#include <exception>

namespace stdlike {

namespace detail {
class Blocker {
 public:
  using Mutex = twist::stdlike::mutex;
  using CondVar = twist::stdlike::condition_variable;
  void Check() {
    std::unique_lock<Mutex> lock(mutex_);
    while (closed_) {
      cond_var_.wait(lock);
    }
  }
  void Open() {
    std::unique_lock<Mutex> lock(mutex_);
    closed_ = false;
    cond_var_.notify_all();
  }

 private:
  Mutex mutex_;
  CondVar cond_var_;
  bool closed_ = true;
};
}  // namespace detail

template <typename T>
class Future {
  template <typename U>
  friend class Promise;

  struct Visitor {
    T operator()(T& a) {
      return std::move(a);
    }
    T operator()(std::exception_ptr& ex) {
      std::rethrow_exception(ex);
      assert(false);
      return std::move(*((T*)this));  // unreachable
    }
  };

 public:
  using Message = std::variant<T, std::exception_ptr>;
  // Non-copyable
  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  // Movable
  Future(Future&&) = default;
  Future& operator=(Future&&) = default;

  // One-shot
  // Wait for result (value or exception)
  T Get() {
    is_written_->Check();
    return std::visit(Visitor(), **message_);
  }

 private:
  Future(std::shared_ptr<detail::Blocker> blocker,
         std::shared_ptr<std::shared_ptr<Message>> message)
      : is_written_(std::move(blocker)), message_(std::move(message)) {
  }

 private:
  std::shared_ptr<detail::Blocker> is_written_;
  std::shared_ptr<std::shared_ptr<Message>> message_;
};

}  // namespace stdlike
