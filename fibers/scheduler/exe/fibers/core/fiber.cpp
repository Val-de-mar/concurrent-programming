#include <exe/fibers/core/fiber.hpp>
#include <exe/fibers/core/stacks.hpp>

#include <twist/util/thread_local.hpp>
#include "api.hpp"

namespace exe::fibers {
static twist::strand::ThreadLocalPtr<Fiber> current_fiber;

//////////////////////////////////////////////////////////////////////

void Fiber::Schedule(executors::Hint hint) {
  executor_.Execute(this, hint);
}

void Fiber::Yield() {
  YieldAwaiter<Fiber> awaiter;
  SetAwaiter(&awaiter);
  Suspend();
}
void Fiber::Run() {
  current_fiber = this;
  awaiter_ = nullptr;

  coroutine_.Resume();
  if (awaiter_ != nullptr) {
    awaiter_->AwaitSuspend(FiberHandle(this));
  } else {
    delete this;
  }
}

void Fiber::Discard() noexcept {
  delete this;
}

Fiber& Fiber::Self() {
  return *current_fiber;
}

Fiber::Fiber(Scheduler& scheduler, Routine routine)
    : executor_(scheduler),
      stack_(AllocateStack()),
      coroutine_(std::move(routine), stack_.View()) {
}

Fiber::~Fiber() {
  ReleaseStack(std::move(stack_));
}

void Fiber::Suspend() {
  coroutine_.Suspend();
}
void Fiber::SetAwaiter(IAwaiter<Fiber>* awaiter) {
  awaiter_ = awaiter;
}

executors::IExecutor& Fiber::GetExecutor() noexcept {
  return executor_;
}

//////////////////////////////////////////////////////////////////////

// API Implementation

void Go(Scheduler& scheduler, Routine routine) {
  auto fiber = new Fiber(scheduler, std::move(routine));
  fiber->Schedule();
}
void Go(Routine routine) {
  Go(Fiber::Self().GetExecutor(), std::move(routine));
}

namespace self {

void Yield() {
  current_fiber->Yield();
}
void Suspend() {
  current_fiber->Suspend();
}

}  // namespace self

}  // namespace exe::fibers
