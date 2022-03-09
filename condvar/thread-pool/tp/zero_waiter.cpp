//
// Created by val-de-mar on 09.03.2022.
//

#include <tp/zero_waiter.hpp>

#include <cassert>

namespace tp {

ZeroWaiter& ZeroWaiter::operator--() {
  std::unique_lock lock(mutex_);
  assert(counter_ != 0);
  --counter_;
  if (counter_ == 0) {
    is_null_.notify_all();
  }
  return *this;
}
ZeroWaiter& ZeroWaiter::operator++() {
  std::unique_lock lock(mutex_);
  ++counter_;
  return *this;
}
void ZeroWaiter::Wait() {
  std::unique_lock lock(mutex_);
  if (counter_ != 0) {
    is_null_.wait(lock);
  }
}
}  // namespace tp