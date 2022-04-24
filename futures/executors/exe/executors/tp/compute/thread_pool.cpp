#include <exe/executors/tp/compute/thread_pool.hpp>

#include <twist/util/thread_local.hpp>

namespace exe::executors::tp::compute {

////////////////////////////////////////////////////////////////////////////////

static twist::util::ThreadLocalPtr<ThreadPool> pool;

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
  std::cout << "here\n";
  //  assert(is_stopped_);
}

void ThreadPool::Execute(Task task) {
  waiter_.IncrementValue();
  if (!tasks_.Put(std::move(task))) {
    waiter_.DecrementValue();
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
  return pool;
}

void ThreadPool::ThreadRoutine() {
  pool = this;
  while (auto task = tasks_.Take()) {
    try {
      task.value()();
    } catch (...) {
    }
    waiter_.DecrementValue();
  }
  pool = nullptr;
}

}  // namespace exe::executors::tp::compute