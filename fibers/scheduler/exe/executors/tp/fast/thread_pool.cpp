#include <exe/executors/tp/fast/thread_pool.hpp>

#include <twist/util/thread_local.hpp>

#include <cassert>

namespace exe::executors::tp::fast {

ThreadPool::ThreadPool(size_t threads)
    : workers_(threads), global_tasks_(threads), size_(threads) {
  for (size_t i = 0; i < threads; ++i) {
    workers_.EmplaceBack(*this, i);
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
  assert(false);
}

void ThreadPool::WaitIdle() {
  //  std::cout << "finish?\n";
  global_tasks_.WaitIdle();
  //  std::cout << "finish\n";
}

void ThreadPool::Stop() {
  for (auto& worker : workers_) {
    worker.Stop();
  }
  //  std::cout << "stoped\n";
  global_tasks_.Stop();
  for (auto& worker : workers_) {
    worker.Join();
  }
  //  std::cout << "waited stopped\n";
}

PoolMetrics ThreadPool::Metrics() const {
  std::abort();
}
size_t ThreadPool::Size() {
  return size_.load();
}

// size_t ThreadPool::GrabFromGlobal(std::span<TaskBase*> out_buffer) {
////  return global_tasks_.Grab(out_buffer, workers_.size());
//}

}  // namespace exe::executors::tp::fast
