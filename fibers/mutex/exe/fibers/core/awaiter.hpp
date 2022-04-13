#pragma once

#include <exe/fibers/core/handle.hpp>

namespace exe::fibers {

template <typename FiberT>
struct IAwaiter {
  // Your AwaitSuspend goes here
  virtual void AwaitSuspend() = 0;
  virtual ~IAwaiter() = default;
};

template <typename FiberT>
struct YieldAwaiter : public IAwaiter<FiberT> {
  virtual void AwaitSuspend() {
    FiberT::Self().Schedule();
  }
};

}  // namespace exe::fibers
