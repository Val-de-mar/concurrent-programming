#pragma once

#include <exe/executors/task.hpp>

#include <twist/stdlike/atomic.hpp>

#include <array>
#include <cassert>
#include <span>

namespace exe::executors::tp::fast {

// Single producer / multiple consumers bounded queue
// for local tasks

template <size_t Capacity>
class WorkStealingQueue {
  struct Slot {
    twist::stdlike::atomic<TaskBase*> item;
  };

 public:
  // Single producer

  bool TryPush(TaskBase* item) {
    uint64_t begin = begin_.load(std::memory_order_acquire);
    uint64_t end = end_.load(std::memory_order_relaxed);
    //    std::cout << "inside " + std::to_string(LowSize()) + "\n";
    if (end >= begin + Capacity) {
      //      std::cout << "overload" + std::to_string(LowSize()) + "\n";
      return false;
    }
    buffer_[end % Capacity].item.store(item, std::memory_order_relaxed);
    end_.fetch_add(1, std::memory_order_release);
    return true;
  }

  // For grabbing from global queue / for stealing
  // Should always succeed
  void PushMany(std::span<TaskBase*> buffer) {
    uint64_t end = end_.load(std::memory_order_relaxed);
    assert(end + buffer.size() <= begin_.load() + Capacity);
    for (auto item : buffer) {
      buffer_[end % Capacity].item.store(item, std::memory_order_relaxed);
      ++end;
    }
    end_.fetch_add(buffer.size(), std::memory_order_release);
  }

  // Multiple consumers

  // Returns nullptr if queue is empty
  TaskBase* TryPop() {
    uint64_t begin;
    while (true) {
      begin = begin_.load(std::memory_order_acquire);
      if (end_.load(std::memory_order_relaxed) == begin) {
        //        std::cout << std::to_string(begin) + " " +
        //        std::to_string(end_.load()) + "error\n";
        return nullptr;
      }

      auto ans = buffer_[begin % Capacity].item.load(std::memory_order_relaxed);
      if (begin_.compare_exchange_weak(begin, begin + 1,
                                       std::memory_order_acq_rel)) {
        //        std::cout << "from buffer " + std::to_string(uint64_t(ans)) +
        //        "\n";
        return ans;
      }
    }
  }

  // For stealing and for offloading to global queue
  // Returns number of tasks in `out_buffer`
  size_t Grab(std::span<TaskBase*> out_buffer) {
    uint64_t begin;
    uint64_t end;
    while (true) {
      begin = begin_.load(std::memory_order_acquire);
      end = end_.load(std::memory_order_acquire);
      if (end <= begin) {
        return 0;
      }
      auto lim = std::min(out_buffer.size(), end - begin);
      for (size_t i = 0; i < lim; ++i) {
        out_buffer[i] = buffer_[(begin + i) % Capacity].item.load(
            std::memory_order_relaxed);
      }

      if (begin_.compare_exchange_strong(begin, begin + lim,
                                         std::memory_order_acq_rel)) {
        return lim;
      }
    }
  }

  size_t LowSize() {
    uint64_t end = end_.load();
    uint64_t beg = begin_.load();
    if (beg >= end) {
      return 0;
    }
    return size_t(end - beg);
  }

 private:
  std::array<Slot, Capacity> buffer_;
  twist::stdlike::atomic<uint64_t> begin_{0};
  twist::stdlike::atomic<uint64_t> end_{0};
};

}  // namespace exe::executors::tp::fast
