#include <exe/executors/tp/fast/thread_pool.hpp>

#include <twist/util/thread_local.hpp>

#include <cassert>

namespace exe::executors::tp::fast {

ThreadPool::ThreadPool(size_t threads)
    : workers_(), global_tasks_(threads), size_(threads) {
  for (size_t i = 0; i < threads; ++i) {
    workers_.emplace_back(*this, i);
  }

  for (auto& worker : workers_) {
    worker.Start();
  }
}

ThreadPool::~ThreadPool() {
}

void ThreadPool::Execute(TaskBase* task, Hint hint) {
  if (Worker::Current() == nullptr || this != ThreadPool::Current()) {
    global_tasks_.PushOne(task);
    coordinator_.ProhibitParking();
    return;
  }
  if (hint == Hint::Next) {
    Worker::Current()->PushToLifoSlot(task);
    return;
  }
  if (hint == Hint::UpToYou) {
    Worker::Current()->PushToLocalQueue(task);
    return;
  }
  if (hint == Hint::Slow) {
    global_tasks_.PushOne(task);
    coordinator_.ProhibitParking();
    return;
  }
  assert(false);
}

void ThreadPool::WaitIdle() {
  global_tasks_.WaitIdle();
  coordinator_.WaitIdle();
}

void ThreadPool::Stop() {
  for (auto& worker : workers_) {
    worker.Stop();
  }
  global_tasks_.Stop();
  coordinator_.AwakeEveryoneForever();
  for (auto& worker : workers_) {
    worker.Join();
  }
}

PoolMetrics ThreadPool::Metrics() const {
  PoolMetrics ans;
  for (auto& worker : workers_) {
    ans.workers_.push_back(worker.metrics_);
  }
  return ans;
}
size_t ThreadPool::Size() {
  return size_.load();
}

}  // namespace exe::executors::tp::fast
