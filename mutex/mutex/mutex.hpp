#pragma once

#include <twist/stdlike/atomic.hpp>

#include <cstdlib>
#include <cstdint>

namespace stdlike {

class Mutex {
 public:
  void Lock() {
    uint32_t ticket = last_waiting_.fetch_add(1);
    while (ticket != free_ticket_.load()) {
      swapper_.FutexWait(0);
    }
    swapper_.store(0);
  }

  void Unlock() {
    free_ticket_.fetch_add(1);
    swapper_.store(1);
    swapper_.FutexWakeAll();
  }

 private:
  twist::stdlike::atomic<uint32_t> last_waiting_{0};
  twist::stdlike::atomic<uint32_t> free_ticket_{0};
  twist::stdlike::atomic<uint32_t> swapper_{0};
};

}  // namespace stdlike
