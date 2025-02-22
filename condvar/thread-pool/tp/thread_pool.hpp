#pragma once

#include <tp/blocking_queue.hpp>
#include <tp/task.hpp>

#include <twist/stdlike/thread.hpp>
#include <twist/stdlike/atomic.hpp>
#include <twist/util/thread_local.hpp>

#include <tp/zero_waiter.hpp>

#include <vector>

namespace tp {

// Fixed-size pool of worker threads

class ThreadPool {
 public:
  explicit ThreadPool(size_t workers);
  ~ThreadPool();

  // Non-copyable
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  // Schedules task for execution in one of the worker threads
  void Submit(Task task);

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
  ZeroWaiter waiter_;
  std::vector<twist::stdlike::thread> workers_;
  UnboundedBlockingQueue<Task> tasks_;
  bool is_stopped_{false};
};

inline ThreadPool* Current() {
  return ThreadPool::Current();
}

}  // namespace tp
