#include <exe/tp/thread_pool.hpp>

#include <twist/util/thread_local.hpp>

namespace exe::tp {

////////////////////////////////////////////////////////////////////////////////

static twist::util::ThreadLocalPtr<ThreadPool> this_pool;

////////////////////////////////////////////////////////////////////////////////

ThreadPool::ThreadPool(size_t workers) {
  workers_.reserve(workers);
  for (size_t i = 0; i < workers; ++i) {
    workers_.emplace_back([this]() {
      ThreadRoutine();
    });
  }
}

ThreadPool::~ThreadPool() {
  assert(is_stopped_);
}

void ThreadPool::Submit(Task task) {
  ++waiter_;
  if (!tasks_.Put(std::move(task))) {
    --waiter_;
  }
}

void ThreadPool::WaitIdle() {
  waiter_.Wait();
}

void ThreadPool::Stop() {
  tasks_.Cancel();
  is_stopped_ = true;
  for (auto& worker : workers_) {
    worker.join();
  }
}

ThreadPool* ThreadPool::Current() {
  return this_pool;
}

void ThreadPool::ThreadRoutine() {
  tp::this_pool = this;
  while (auto task = tasks_.Take()) {
    try {
      task.value()();
    } catch (...) {
    }
    --(waiter_);
  }
  tp::this_pool = nullptr;
}

}  // namespace exe::tp
