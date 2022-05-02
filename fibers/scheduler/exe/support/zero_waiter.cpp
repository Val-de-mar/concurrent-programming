////
//// Created by val-de-mar on 09.03.2022.
////
//
//#include <exe/support/zero_waiter.hpp>
//
//#include <cassert>
//
// namespace exe::support {
//
// ZeroWaiter::ZeroWaiter(size_t init_value) : counter_(init_value) {
//}
//
// ZeroWaiter& ZeroWaiter::DecrementValue() {
//  std::unique_lock lock(mutex_);
//  assert(counter_ != 0);
//  --counter_;
//  if (counter_ == 0) {
//    is_null_.notify_all();
//  }
//  return *this;
//}
// ZeroWaiter& ZeroWaiter::IncrementValue() {
//  std::unique_lock lock(mutex_);
//  ++counter_;
//  return *this;
//}
// void ZeroWaiter::Wait() {
//  std::unique_lock lock(mutex_);
//  while (counter_ != 0) {
//    is_null_.wait(lock);
//  }
//}
//
// uint32_t ZeroWaiter::GetVal() {
//  return counter_;
//}
//}  // namespace exe::support