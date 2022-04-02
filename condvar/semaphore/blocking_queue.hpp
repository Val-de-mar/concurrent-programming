#pragma once

#include "tagged_semaphore.hpp"

#include <deque>

namespace solutions {

// Bounded Blocking Multi-Producer/Multi-Consumer (MPMC) Queue

template <typename T>
class BlockingQueue {
 public:
  explicit BlockingQueue(size_t capacity)
      : free_space_(capacity), occupied_space_(0), writing_rights_(1) {
  }

  // Inserts the specified element into this queue,
  // waiting if necessary for space to become available.
  void Put(T value) {
    free_space_.Acquire();
    writing_rights_.Acquire();
    queue_.push_back(std::move(value));
    writing_rights_.Release();
    occupied_space_.Release();
  }

  // Retrieves and removes the head of this queue,
  // waiting if necessary until an element becomes available
  T Take() {
    occupied_space_.Acquire();
    writing_rights_.Acquire();
    auto ans = std::move(queue_.front());
    queue_.pop_front();
    writing_rights_.Release();
    free_space_.Release();
    return ans;
  }

 private:
  Semaphore free_space_;
  Semaphore occupied_space_;
  Semaphore writing_rights_;
  std::deque<T> queue_;
  // Buffer
};

}  // namespace solutions
