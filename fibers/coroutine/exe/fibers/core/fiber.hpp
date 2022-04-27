#pragma once

#include <exe/fibers/core/api.hpp>
#include <exe/coroutine/impl.hpp>
#include <context/stack.hpp>

namespace exe::fibers {

// Fiber = Stackful coroutine + Scheduler (Thread pool)

class Fiber {
  struct FiberRunner : tp::TaskBlock {
    void operator()() override;
    explicit FiberRunner(Fiber* owner) : owner_(owner) {
    }

   private:
    Fiber* owner_;
  };

 public:
  // ~ System calls
  void Schedule();
  void Yield();

  static Fiber& Self();

  ~Fiber();

 private:
  Fiber(Scheduler& scheduler, Routine routine);
  // Task
  void Step();

 private:
  Scheduler& scheduler_;
  context::Stack stack_;
  coroutine::CoroutineImpl coroutine_;
  FiberRunner runner_;

  friend void Go(Scheduler& /*scheduler*/, Routine /*routine*/);
  friend struct FiberRunner;
};

}  // namespace exe::fibers