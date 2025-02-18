#pragma once

#include <exe/fibers/core/api.hpp>
#include <exe/fibers/core/awaiter.hpp>
#include <exe/support/spinlock.hpp>

#include <twist/stdlike/atomic.hpp>

#include <wheels/intrusive/list.hpp>
#include <wheels/support/assert.hpp>

namespace exe::fibers {

template <typename T>
class FutexLike;

template <typename FiberT, typename T>
struct FutexAwaiter
    : public IAwaiter<FiberT>,
      public wheels::IntrusiveListNode<FutexAwaiter<FiberT, T>> {
  using Node = wheels::IntrusiveListNode<FutexAwaiter<FiberT, T>>;
  FutexAwaiter(FutexLike<T>* futex, T old_value)
      : futex_(futex),
        handle_(FiberHandle::Invalid()),
        old_value_(std::move(old_value)) {
    //    std::cout << "made futex " << this << std::endl;
  }

  void AwaitSuspend(FiberHandle handle) override;
  void Awake();
  FutexLike<T>* futex_;
  FiberHandle handle_;
  T old_value_;
  virtual ~FutexAwaiter() {
    this->Unlink();
    //    std::cout << "destroyed futex" << this << std::endl;
  }
};

template <typename T>
class FutexLike {
 public:
  explicit FutexLike(twist::stdlike::atomic<T>& cell) : cell_(cell) {
  }

  ~FutexLike() {
    std::unique_lock guard(lock_);
    //     std::cerr << "wanna die " << this << "\n";

    assert(queue_.IsEmpty());
  }

  void ParkIfEqual(T old) {
    std::unique_lock guard(lock_);
    FutexAwaiter<Fiber, T> awaiter(this, old);
    Fiber::Self().SetAwaiter(&awaiter);
    guard.unlock();
    self::Suspend();
    guard.lock();
  }

  void WakeOne() {
    //    std::cerr << "taking\n";
    std::unique_lock guard(lock_);
    //    std::cerr << "taken\n";
    if (!queue_.IsEmpty()) {
      auto node = queue_.PopFront();
      //      assert(node->fiber_ != &Fiber::Self());
      node->Awake();
    }
  }

  void WakeAll() {
    std::unique_lock guard(lock_);
    // std::cerr << "try wake all " << this << "\n";
    while (!queue_.IsEmpty()) {
      auto node = queue_.PopFront();
      assert(node->fiber_ != &Fiber::Self());
      node->Awake();
    }
  }

 private:
  wheels::IntrusiveList<FutexAwaiter<Fiber, T>> queue_;
  support::SpinLock lock_;
  twist::stdlike::atomic<T>& cell_;

  friend struct FutexAwaiter<Fiber, T>;
};

template <typename FiberT, typename T>
void FutexAwaiter<FiberT, T>::AwaitSuspend(FiberHandle handle) {
  handle_ = handle;
  std::unique_lock guard(futex_->lock_);
  futex_->queue_.PushBack(this);
  if (futex_->cell_.load() != old_value_) {
    this->Unlink();
    handle.Schedule();
  }
}

template <typename FiberT, typename T>
void FutexAwaiter<FiberT, T>::Awake() {
  // std::cerr << "awaken " << futex_ << "\n";
  this->Unlink();
  handle_.Schedule();
}

}  // namespace exe::fibers
