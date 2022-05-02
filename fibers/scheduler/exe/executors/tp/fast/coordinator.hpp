#pragma once

#include <twist/stdlike/atomic.hpp>

namespace exe::executors::tp::fast {

// Coordinates workers (stealing, parking)

class Coordinator {
 public:
  void TryParkMe();

 private:
  twist::stdlike::atomic<size_t> active_{0};
  //  twist::stdlike::atomic<size_t> stealing_{0};
  twist::stdlike::atomic<uint32_t> unfinished_;
  // ???
};

}  // namespace exe::executors::tp::fast
