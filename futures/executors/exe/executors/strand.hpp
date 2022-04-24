#pragma once

#include <exe/executors/executor.hpp>
#include <twist/stdlike/atomic.hpp>

namespace exe::executors {

// Strand (serial executor, asynchronous mutex)
// Executes (via underlying executor) tasks
// non-concurrently and in FIFO order

class Strand : public IExecutor {
  struct TaskNode {
    twist::stdlike::atomic<TaskNode*> prev_;
    Task task_;
    TaskNode* Reverse();
  };

  void Schedule();

 public:
  explicit Strand(IExecutor& underlying);

  // IExecutor
  void Execute(Task task) override;

 private:
  IExecutor& slave_;
  twist::stdlike::atomic<TaskNode*> tasks_{nullptr};
  twist::stdlike::atomic<bool> reschedule_{true};
};

}  // namespace exe::executors