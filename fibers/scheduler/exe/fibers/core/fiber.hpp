#pragma once

#include <exe/fibers/core/api.hpp>
#include <exe/coroutine/impl.hpp>
#include <exe/fibers/core/awaiter.hpp>
#include <context/stack.hpp>
namespace exe::fibers {

// Fiber = Stackful coroutine + Scheduler (Thread pool)

class Fiber : public executors::TaskBase {
 public:
  // ~ System calls
  void Schedule(executors::Hint hint = executors::Hint::UpToYou);
  void Yield();
  void Suspend();
  void SetAwaiter(IAwaiter<Fiber>*);

  void Run() override;
  void Discard() noexcept override;

  executors::IExecutor& GetExecutor() noexcept;

  static Fiber& Self();

  ~Fiber() override;

 private:
  Fiber(Scheduler& scheduler, Routine routine);

 private:
  executors::IExecutor& executor_;
  context::Stack stack_;
  coroutine::CoroutineImpl coroutine_;
  IAwaiter<Fiber>* awaiter_;

  friend void Go(Scheduler& /*scheduler*/, Routine /*routine*/);
};

}  // namespace exe::fibers