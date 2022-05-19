#include <exe/executors/tp/fast/coordinator.hpp>

namespace exe::executors::tp::fast {

void Coordinator::TryParkMe(Worker& worker) {
  std::unique_lock lock(balance_changes_);
  if (permission_to_sleep_ == Allowed) {
    worker.Unlink();
    waiter_.DecrementValue();
    sleeper_.wait(lock);
    AcquireUnsafe(worker);
  }
}
void Coordinator::AcquireUnsafe(Worker& worker) {
  waiter_.IncrementValue();
  active_.PushFront(&worker);
  worker.MakeActive();
  RebalanceUnsafe();
}

void Coordinator::Release(Worker& worker) {
  std::lock_guard guard(balance_changes_);
  worker.Unlink();
  waiter_.DecrementValue();
  RebalanceUnsafe();
}

void Coordinator::RebalanceUnsafe() {
  while (active_.Size() > passive_.Size()) {
    auto node = active_.PopBack();
    if (node != nullptr) {
      node->MakePassive();
      passive_.PushFront(node);
    } else {
      break;
    }
  }
  while (active_.Size() < passive_.Size()) {
    auto node = passive_.PopBack();
    if (node != nullptr) {
      node->MakeActive();
      active_.PushFront(node);
    } else {
      break;
    }
  }
}
void Coordinator::ProhibitParking() {
  std::lock_guard guard(balance_changes_);
  if (permission_to_sleep_ != PermanentlyProhibited) {
    permission_to_sleep_ = TemporaryProhibited;
  }
  sleeper_.notify_one();
}
void Coordinator::AllowParking() {
  std::lock_guard guard(balance_changes_);
  //  std::cout << "allow\n";
  if (permission_to_sleep_ != PermanentlyProhibited) {
    permission_to_sleep_ = Allowed;
  }
}
void Coordinator::AwakeOne() {
  std::lock_guard guard(balance_changes_);
  sleeper_.notify_one();
}
void Coordinator::AwakeEveryone() {
  std::lock_guard guard(balance_changes_);
  sleeper_.notify_all();
}
void Coordinator::AwakeEveryoneForever() {
  std::lock_guard guard(balance_changes_);
  permission_to_sleep_ = PermanentlyProhibited;
  sleeper_.notify_all();
}
void Coordinator::WaitIdle() {
  waiter_.Wait();
}
Worker& Coordinator::GiveVictim(size_t seed) {
  std::lock_guard guard(balance_changes_);
  auto place = seed % (active_.Size() + passive_.Size());
  if (place < active_.Size()) {
    auto iter = active_.begin();
    std::advance(iter, place);
    return *iter;
  } else {
    auto iter = passive_.begin();
    std::advance(iter, place - active_.Size());
    return *iter;
  }
}
}  // namespace exe::executors::tp::fast
