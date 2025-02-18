#include <exe/fibers/core/fiber.hpp>
#include <exe/fibers/core/stacks.hpp>

#include <twist/util/thread_local.hpp>
#include <twist/stdlike/mutex.hpp>

#include <asio/steady_timer.hpp>

using namespace std::chrono_literals;

namespace exe::fibers {

static twist::strand::ThreadLocalPtr<Fiber> current_fiber;
static twist::strand::ThreadLocalPtr<asio::steady_timer> curr_timer;
static twist::strand::ThreadLocal<Millis> curr_delay;

//////////////////////////////////////////////////////////////////////

// twist::stdlike::mutex m;

void Fiber::Schedule() {
  if (state_.load() != Running) {
    scheduler_.post([this]() {
      Step();
    });
  } else {
    delete this;
  }
}

void Fiber::Yield() {
  SetState(Waiting);
  coroutine_.Suspend();
}

void Fiber::SleepFor(Millis delay) {
  SetState(Sleeping);

  asio::steady_timer timer(scheduler_);
  curr_timer = &timer;
  *curr_delay = delay;

  coroutine_.Suspend();
}

void Fiber::Step() {
  current_fiber = this;
  curr_timer = nullptr;
  SetState(Running);
  coroutine_.Resume();
  if (state_.load() == Sleeping) {
    curr_timer->expires_after(*curr_delay);
    curr_timer->async_wait([this](std::error_code /*ec*/) {
      exe::fibers::current_fiber = this;
      SetState(Waiting);
      Schedule();
    });
  } else {
    Schedule();
  }
}

Fiber& Fiber::Self() {
  return *current_fiber;
}

// void Fiber::TrySuicide() {
//   if (running_.fetch_sub(1) == 1) {
//     delete this;
//   }
// }

Fiber::Fiber(Scheduler& scheduler, Routine routine)
    : scheduler_(scheduler),
      stack_(AllocateStack()),
      coroutine_(std::move(routine), stack_.View()),
      state_(Rescheduling) {
}

Fiber::~Fiber() {
  ReleaseStack(std::move(stack_));
}

//////////////////////////////////////////////////////////////////////

// API Implementation

void Go(Scheduler& scheduler, Routine routine) {
  auto fiber = new Fiber(scheduler, std::move(routine));
  // std::cerr << "New " << fiber << "\n";
  fiber->Schedule();
}

void Go(Routine routine) {
  Go(current_fiber->scheduler_, std::move(routine));
}
void Fiber::SetState(Fiber::State state) {
  state_.store(state);
}
// void Fiber::Suspend() {
// }
// void Fiber::Resume() {
// }
// void Fiber::Dispatch() {
// }

// twist::stdlike::mutex m;

namespace self {

void Yield() {
  // std::cerr << "Yie " << current_fiber << "\n";
  current_fiber->Yield();
}

void SleepFor(Millis delay) {
  // std::cerr << current_fiber << "\n";
  current_fiber->SleepFor(delay);
}

}  // namespace self

}  // namespace exe::fibers
