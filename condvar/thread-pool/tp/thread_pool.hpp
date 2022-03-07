#pragma once

#include <tp/blocking_queue.hpp>
#include <tp/task.hpp>

#include <twist/stdlike/thread.hpp>
#include <twist/stdlike/atomic.hpp>
#include <twist/stdlike/atomic.hpp>
#include <twist/util/thread_local.hpp>

namespace tp {

// Fixed-size pool of worker threads

class ThreadPool {
  class NullWaiter {
   public:
    NullWaiter& operator--();
    NullWaiter& operator++();
    void Wait();

   private:
    uint32_t counter_ = 0;
    twist::stdlike::mutex mutex_;
    twist::stdlike::condition_variable is_null_;
  };

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
  NullWaiter waiter_;
  std::vector<twist::stdlike::thread> workers_;
  UnboundedBlockingQueue<Task> tasks_;
  twist::stdlike::atomic<bool> is_stopped_{false};
};

inline ThreadPool* Current() {
  return ThreadPool::Current();
}

}  // namespace tp
