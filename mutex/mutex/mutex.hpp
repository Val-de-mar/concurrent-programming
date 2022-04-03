#pragma once

#include <twist/stdlike/atomic.hpp>

#include <cstdlib>
#include <cstdint>

namespace stdlike {

class Mutex {
 public:
  void Lock() {
    while (access_.exchange(1) != 0u) {
      access_.FutexWait(1);
    }
  }

  void Unlock() {
    access_.store(0);
    access_.notify_one();
  }

 private:
  twist::stdlike::atomic<uint32_t> access_{0};
};

}  // namespace stdlike