#pragma once

#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>

#include <wheels/support/compiler.hpp>

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

struct ExceptionIsolator {
  std::exception_ptr exception_;
};

template <typename T>
struct FutureControlBlock {
  Blocker is_ready_;
  std::variant<T, ExceptionIsolator> message_ = ExceptionIsolator{nullptr};
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
    T operator()(detail::ExceptionIsolator& ex) {
      std::rethrow_exception(ex.exception_);
      WHEELS_UNREACHABLE();
    }
  };

 public:
  // Non-copyable
  Future(const Future&) = delete;
  Future& operator=(const Future&) = delete;

  // Movable
  Future(Future&&) = default;
  Future& operator=(Future&&) = default;

  // One-shot
  // Wait for result (value or exception)
  T Get() {
    connection_->is_ready_.Check();
    return std::visit(Visitor(), connection_->message_);
  }

 private:
  explicit Future(std::shared_ptr<detail::FutureControlBlock<T>> connection)
      : connection_(std::move(connection)) {
  }

 private:
  std::shared_ptr<detail::FutureControlBlock<T>> connection_;
};

}  // namespace stdlike