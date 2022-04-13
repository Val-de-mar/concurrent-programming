#pragma once

#include <exe/fibers/core/api.hpp>
#include <exe/coroutine/impl.hpp>
#include <exe/fibers/core/awaiter.hpp>
#include <context/stack.hpp>
namespace exe::fibers {

// Fiber = Stackful coroutine + Scheduler (Thread pool)

class Fiber {
 public:
  // ~ System calls
  void Schedule();
  void Yield();
  void Suspend();
  void SetAwaiter(IAwaiter<Fiber>*);

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
  IAwaiter<Fiber>* awaiter_;

  friend void Go(Scheduler& /*scheduler*/, Routine /*routine*/);
};

}  // namespace exe::fibers
