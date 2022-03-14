#include <exe/coroutine/impl.hpp>

#include <wheels/support/assert.hpp>
#include <wheels/support/compiler.hpp>

namespace exe::coroutine {

CoroutineImpl::CoroutineImpl(Routine routine, wheels::MutableMemView stack)
    : routine_(std::move(routine)), stack_(stack) {
  running_point_.Setup(stack_, this);
  SetState(State::Waiting);
}

void CoroutineImpl::Run() {
  try {
    auto runner = std::move(routine_);
    runner();
  } catch (...) {
    caught_ = std::current_exception();
    SetState(State::ExceptionCaught);
  }
  SetState(State::Terminated);
  running_point_.ExitTo(return_point_);

  WHEELS_UNREACHABLE();
}

void CoroutineImpl::Resume() {
  SetState(State::Running);
  return_point_.SwitchTo(running_point_);
  if (caught_) {
    std::rethrow_exception(caught_);
  }
}

void CoroutineImpl::Suspend() {
  SetState(State::Waiting);
  running_point_.SwitchTo(return_point_);
}

bool CoroutineImpl::IsCompleted() const {
  return (state_ == State::Terminated);
}
void CoroutineImpl::SetState(State to) {
  state_ = to;
}

}  // namespace exe::coroutine
