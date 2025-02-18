#include <exe/support/zero_waiter.hpp>

#include <cassert>

#ifndef CLION_DEBUG
namespace exe::support {

ZeroWaiter& ZeroWaiter::DecrementValue() {
  std::unique_lock lock(mutex_);
  assert(counter_ != 0);
  --counter_;
  if (counter_ == 0) {
    is_null_.notify_all();
  }
  return *this;
}
ZeroWaiter& ZeroWaiter::IncrementValue() {
  std::unique_lock lock(mutex_);
  ++counter_;
  return *this;
}
void ZeroWaiter::Wait() {
  std::unique_lock lock(mutex_);
  while (counter_ != 0) {
    is_null_.wait(lock);
  }
}
}  // namespace exe::support
#endif