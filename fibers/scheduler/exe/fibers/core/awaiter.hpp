#pragma once

#include <exe/fibers/core/handle.hpp>

namespace exe::fibers {

template <typename FiberT>
struct IAwaiter {
  // Your AwaitSuspend goes here
  virtual void AwaitSuspend(FiberHandle handle) = 0;
  virtual ~IAwaiter() = default;
};

template <typename FiberT>
struct YieldAwaiter : public IAwaiter<FiberT> {
  void AwaitSuspend(FiberHandle handle) override {
    handle.Schedule(executors::Hint::Slow);
  }
};

}  // namespace exe::fibers
