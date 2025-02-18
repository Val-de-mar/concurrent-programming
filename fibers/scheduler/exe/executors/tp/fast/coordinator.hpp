#pragma once

#include <exe/executors/tp/fast/worker.hpp>
#include <exe/support/zero_waiter.hpp>

#include <twist/stdlike/atomic.hpp>
#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>

#include <wheels/intrusive/list.hpp>

namespace exe::executors::tp::fast {

// Coordinates workers (stealing, parking)

class Coordinator {
 public:
  enum SleepingState {
    Allowed,
    TemporaryProhibited,
    PermanentlyProhibited,
  };

  void AcquireUnsafe(Worker& worker);
  void Release(Worker& worker);

  void TryParkMe(Worker& worker);

  void ProhibitParking();
  void AllowParking();
  void AwakeOne();
  void AwakeEveryone();
  void AwakeEveryoneForever();

  Worker& GiveVictim(size_t seed);

  void WaitIdle();

 private:
  void RebalanceUnsafe();

 private:
  twist::stdlike::mutex balance_changes_;
  twist::stdlike::condition_variable sleeper_;
  support::ZeroWaiter waiter_;

  wheels::IntrusiveList<Worker> active_;
  wheels::IntrusiveList<Worker> passive_;
  wheels::IntrusiveList<Worker> sleeping_;

  SleepingState permission_to_sleep_ = SleepingState::Allowed;
};

}  // namespace exe::executors::tp::fast
