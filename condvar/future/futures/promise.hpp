#pragma once

#include <futures/future.hpp>

#include <memory>
#include <mutex>

namespace stdlike {

template <typename T>
class Promise {
 public:
  Promise()
      : is_ready_(std::make_shared<detail::Blocker>()),
        message_(
            std::make_shared<std::shared_ptr<typename Future<T>::Message>>()) {
  }

  // Non-copyable
  Promise(const Promise&) = delete;
  Promise& operator=(const Promise&) = delete;

  // Movable
  Promise(Promise&&) = default;
  Promise& operator=(Promise&&) = default;

  // One-shot
  Future<T> MakeFuture() {
    return Future<T>(is_ready_, message_);
  }

  // One-shot
  // Fulfill promise with value
  void SetValue(T value) {
    *message_ = std::make_shared<typename Future<T>::Message>(std::move(value));
    is_ready_->Open();
  }

  // One-shot
  // Fulfill promise with exception
  void SetException(std::exception_ptr ex) {
    *message_ = std::make_shared<typename Future<T>::Message>(std::move(ex));
    is_ready_->Open();
  }

 private:
  std::shared_ptr<detail::Blocker> is_ready_;
  std::shared_ptr<std::shared_ptr<typename Future<T>::Message>> message_;
};

}  // namespace stdlike
