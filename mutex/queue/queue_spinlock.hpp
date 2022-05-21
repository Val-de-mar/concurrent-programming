#pragma once

#include <twist/stdlike/atomic.hpp>
#include <twist/util/spin_wait.hpp>

namespace spinlocks {

/*  Scalable Queue SpinLock
 *
 *  Usage:
 *
 *  QueueSpinLock qspinlock;
 *  {
 *    QueueSpinLock::Guard guard(qspinlock);  // <-- Acquire
 *    // Critical section
 *  }  // <-- Release
 */

class QueueSpinLock {
  struct SpinNode {
    twist::stdlike::atomic<SpinNode*> next_;
    twist::stdlike::atomic<bool> not_changed_;

    SpinNode() : next_{nullptr}, not_changed_{true} {
    }
  };

 public:
  QueueSpinLock() : end_{nullptr} {
  }

  class Guard {
    friend class QueueSpinLock;

   public:
    explicit Guard(QueueSpinLock& spinlock) : spinlock_(spinlock) {
      spinlock_.Acquire(this);
    }

    ~Guard() {
      spinlock_.Release(this);
    }

   private:
    QueueSpinLock& spinlock_;
    SpinNode node_;
  };

 private:
  void Acquire(Guard* guard) {
    auto prev = end_.exchange(&(guard->node_));
    if (prev != nullptr) {
      prev->next_.store(&(guard->node_));
      twist::util::SpinWait wait;
      while (guard->node_.not_changed_.load()) {
        wait.Spin();
      }
    }
  }

  void Release(Guard* owner) {
    SpinNode* copy = &owner->node_;
    end_.compare_exchange_strong(copy, nullptr);
    if (copy != &owner->node_) {
      twist::util::SpinWait wait;
      while (owner->node_.next_.load() == nullptr) {
        wait.Spin();
      }
      owner->node_.next_.load()->not_changed_.store(false);
    }
  }

 private:
  twist::stdlike::atomic<SpinNode*> end_;
};

}  // namespace spinlocks
