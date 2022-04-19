#pragma once

#include <exe/executors/executor.hpp>
#include <exe/executors/tp/zero_waiter.hpp>
#include <exe/executors/tp/blocking_queue.hpp>

namespace exe::executors {

// Fixed-size pool of worker threads

class ThreadPool : public IExecutor {
 public:
  explicit ThreadPool(size_t workers);
  ~ThreadPool();

  // Non-copyable
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // IExecutor
  // Schedules task for execution in one of the worker threads
  void Execute(Task task);

  // Waits until outstanding work count has reached zero
  void WaitIdle();

  // Stops the worker threads as soon as possible
  // Pending tasks will be discarded
  void Stop();

  // Locates current thread pool from worker thread
  static ThreadPool* Current();

 private:
  void ThreadRoutine();

 private:
  exe::detail::ZeroWaiter waiter_;
  std::vector<twist::stdlike::thread> workers_;
  exe::detail::UnboundedBlockingQueue<Task> tasks_;
  bool is_stopped_{false};
};

}  // namespace exe::executors
