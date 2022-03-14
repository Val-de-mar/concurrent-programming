#pragma once

#include <exe/fibers/core/api.hpp>
#include <exe/coroutine/impl.hpp>
#include <context/stack.hpp>

namespace exe::fibers {

// Fiber = Stackful coroutine + Scheduler (Thread pool)

class Fiber {
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

  friend void Go(Scheduler& /*scheduler*/, Routine /*routine*/);
};

}  // namespace exe::fibers
