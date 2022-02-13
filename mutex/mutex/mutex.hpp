#pragma once

#include <twist/stdlike/atomic.hpp>

#include <cstdlib>
#include <cstdint>

namespace stdlike {

class Mutex {
 public:
  void Lock() {
    uint32_t ticket = last_waiting_.fetch_add(1);
    uint32_t prev = free_ticket_.load();
    while (ticket != prev) {
      free_ticket_.FutexWait(prev);
      prev = free_ticket_.load();
    }
  }

  void Unlock() {
    free_ticket_.fetch_add(1);
    free_ticket_.FutexWakeAll();
  }

 private:
  twist::stdlike::atomic<uint32_t> last_waiting_{0};
  twist::stdlike::atomic<uint32_t> free_ticket_{0};
};

}  // namespace stdlike
