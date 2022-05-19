#pragma once

#include <cstdlib>
#include <vector>

namespace exe::executors::tp::fast {

struct WorkerMetrics {
  size_t index_;
  uint64_t executed_ = 0;
  uint64_t discarded_ = 0;
  uint64_t from_lifo_ = 0;
  uint64_t stolen_ = 0;
  uint64_t grabbed_from_global_ = 0;
  uint64_t offloaded_ = 0;
  explicit WorkerMetrics(size_t index) : index_(index) {
  }
};

struct PoolMetrics {
  std::vector<WorkerMetrics> workers_{};
};

}  // namespace exe::executors::tp::fast
