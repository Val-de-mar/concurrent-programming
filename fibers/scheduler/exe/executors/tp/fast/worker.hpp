#pragma once

#include <exe/executors/task.hpp>

#include <exe/executors/tp/fast/metrics.hpp>
#include <exe/executors/tp/fast/queues/work_stealing_queue.hpp>

#include <twist/stdlike/atomic.hpp>
#include <twist/stdlike/thread.hpp>
#include <twist/stdlike/random.hpp>

#include <cstdlib>
#include <optional>
#include <random>
#include <span>

namespace exe::executors::tp::fast {

class ThreadPool;

class Worker {
 private:
  static const size_t kLocalQueueCapacity = 256;

 public:
  Worker(ThreadPool& host, size_t index);

  void Start();
  void Join();

  // Submit task
  void PushToLocalQueue(TaskBase* task);
  void PushToLifoSlot(TaskBase* task);

  // Steal from this worker
  size_t StealTasks(std::span<TaskBase*> out_buffer);

  // Wake parked worker
  void Wake();

  static Worker* Current();

  WorkerMetrics Metrics() const {
    return metrics_;
  }

  ThreadPool& Host() const {
    return host_;
  }

  void Stop() {
    stopped_.store(true);
  }

 private:
  void OffloadTasksToGlobalQueue(TaskBase* overflow);

  TaskBase* TryStealTasks(size_t series);
  TaskBase* GrabTasksFromGlobalQueue();
  TaskBase* TryPickNextTask();
  TaskBase* TryPickTaskBeforePark();

  // Blocking
  TaskBase* PickNextTask();

  // Run Loop
  void Work();

 private:
  ThreadPool& host_;
  const size_t index_;

  // Worker thread
  std::optional<twist::stdlike::thread> thread_;

  // Local queue
  WorkStealingQueue<kLocalQueueCapacity> local_tasks_;

  // For work stealing
  std::mt19937_64 twister_{twist::stdlike::random_device()()};
  size_t cycle_{0};

  // LIFO slot
  TaskBase* lifo_slot_ = nullptr;
  size_t lifo_usage_{0};
  const static size_t kLifoLimit = 19;

  // Parking lot
  twist::stdlike::atomic<uint32_t> wakeups_{0};

  twist::stdlike::atomic<bool> stopped_{false};

  WorkerMetrics metrics_;
};

}  // namespace exe::executors::tp::fast
