#include <exe/fibers/core/fiber.hpp>
#include <exe/fibers/core/stacks.hpp>

#include <twist/util/thread_local.hpp>

namespace exe::fibers {

static twist::strand::ThreadLocalPtr<Fiber> current_fiber;

//////////////////////////////////////////////////////////////////////

void Fiber::FiberRunner::operator()() {
  owner_->Step();
}

void Fiber::Schedule() {
  scheduler_.Submit(tp::Task(static_cast<tp::TaskBlock*>(&runner_)));
}

void Fiber::Yield() {
  coroutine_.Suspend();
}

void Fiber::Step() {
  exe::fibers::current_fiber = this;
  coroutine_.Resume();
  if (!coroutine_.IsCompleted()) {
    Schedule();
  } else {
    delete this;
  }
  exe::fibers::current_fiber = nullptr;
}

Fiber& Fiber::Self() {
  return *current_fiber;
}

Fiber::Fiber(Scheduler& scheduler, Routine routine)
    : scheduler_(scheduler),
      stack_(AllocateStack()),
      coroutine_(std::move(routine), stack_.View()),
      runner_(this) {
}

Fiber::~Fiber() {
  ReleaseStack(std::move(stack_));
}

//////////////////////////////////////////////////////////////////////

// API Implementation

void Go(Scheduler& scheduler, Routine routine) {
  auto fiber = new Fiber(scheduler, std::move(routine));
  fiber->Schedule();
}

void Go(Routine routine) {
  Go(*tp::ThreadPool::Current(), std::move(routine));
}

namespace self {

void Yield() {
  current_fiber->Yield();
}

}  // namespace self

}  // namespace exe::fibers