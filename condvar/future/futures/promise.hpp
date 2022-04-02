#pragma once

#include <futures/future.hpp>

#include <memory>
#include <mutex>

namespace stdlike {

template <typename T>
class Promise {
 public:
  Promise() : connection_(std::make_shared<detail::FutureControlBlock<T>>()) {
  }

  // Non-copyable
  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  // Movable
  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  // One-shot
  Future<T> MakeFuture() {
    return Future<T>(connection_);
  }

  // One-shot
  // Fulfill promise with value
  void SetValue(T value) {
    connection_->message_ = std::move(value);
    connection_->is_ready_.Open();
  }

  // One-shot
  // Fulfill promise with exception
  void SetException(std::exception_ptr ex) {
    connection_->message_ = detail::ExceptionIsolator{std::move(ex)};
    connection_->is_ready_.Open();
  }

 private:
  std::shared_ptr<detail::FutureControlBlock<T>> connection_;
};

}  // namespace stdlike