#pragma once

#include <exe/fibers/core/api.hpp>
#include <exe/coroutine/impl.hpp>

#include <context/stack.hpp>
#include <twist/stdlike/atomic.hpp>

namespace exe::fibers {

// Fiber = Stackful coroutine + Scheduler

class Fiber {
  enum State {
    Waiting,
    Rescheduling,
    Sleeping,
    Running,
  };

 public:
  // ~ System calls

  void Schedule();

  void Yield();
  void SleepFor(Millis delay);

  static Fiber& Self();

  ~Fiber();

 private:
  Fiber(Scheduler& scheduler, Routine routine);
  // Task
  void Step();

  void SetState(State);

  void TrySuicide();

 private:
  Scheduler& scheduler_;
  context::Stack stack_;
  coroutine::CoroutineImpl coroutine_;
  twist::stdlike::atomic<State> state_;
  //  twist::stdlike::atomic<uint64_t> running_;

  friend void Go(Scheduler& /*scheduler*/, Routine /*routine*/);
  friend void Go(Routine /*routine*/);
};

}  // namespace exe::fibers
