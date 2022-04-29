#pragma once

#include <array>
#include <span>
#include <twist/stdlike/atomic.hpp>

namespace lockfree {

// Single-Producer / Multi-Consumer Bounded Ring Buffer

template <typename T, size_t Capacity>
class WorkStealingQueue {
  struct Slot {
    twist::stdlike::atomic<T*> item;
  };

 public:
  bool TryPush(T* item) {
    uint64_t begin = begin_.load(std::memory_order_acquire);
    uint64_t end = end_.load(std::memory_order_relaxed);
    if (end >= begin + Capacity) {
      return false;
    }
    buffer_[end % Capacity].item.store(item, std::memory_order_relaxed);
    end_.fetch_add(1, std::memory_order_release);
    return true;
  }

  // Returns nullptr if queue is empty
  T* TryPop() {
    uint64_t begin;
    while (true) {
      begin = begin_.load(std::memory_order_acquire);
      if (end_.load(std::memory_order_relaxed) == begin) {
        return nullptr;
      }

      T* ans = buffer_[begin % Capacity].item.load(std::memory_order_relaxed);
      if (begin_.compare_exchange_weak(begin, begin + 1,
                                       std::memory_order_acq_rel)) {
        return ans;
      }
    }
  }

  // Returns number of tasks
  size_t Grab(std::span<T*> out_buffer) {
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

 private:
  std::array<Slot, Capacity> buffer_;
  twist::stdlike::atomic<uint64_t> begin_{0};
  twist::stdlike::atomic<uint64_t> end_{0};
};

}  // namespace lockfree
