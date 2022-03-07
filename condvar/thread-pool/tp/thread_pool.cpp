#include <tp/thread_pool.hpp>

#include <twist/util/thread_local.hpp>

namespace tp {

////////////////////////////////////////////////////////////////////////////////

static twist::util::ThreadLocalPtr<ThreadPool> this_pool;

////////////////////////////////////////////////////////////////////////////////

ThreadPool::ThreadPool(size_t workers) {
  workers_.reserve(workers);
  for (size_t i = 0; i < workers; ++i) {
    workers_.emplace_back([pool = this]() {
      tp::this_pool = pool;
      while (true) {
        auto task = pool->tasks_.Take();
        if (task) {
          try {
            task.value()();
          } catch (...) {
          }
          --(pool->waiter_);
        } else {
          return;
        }
      }
      tp::this_pool = nullptr;
    });
  }
}

ThreadPool::~ThreadPool() {
  assert(is_stopped_.load());
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
  is_stopped_.store(true);
  for (auto& worker : workers_) {
    worker.join();
  }
}

ThreadPool* ThreadPool::Current() {
  return this_pool;
}

ThreadPool::NullWaiter& ThreadPool::NullWaiter::operator--() {
  std::unique_lock lock(mutex_);
  assert(counter_ != 0);
  --counter_;
  if (counter_ == 0) {
    is_null_.notify_all();
  }
  return *this;
}
ThreadPool::NullWaiter& ThreadPool::NullWaiter::operator++() {
  std::unique_lock lock(mutex_);
  ++counter_;
  return *this;
}
void ThreadPool::NullWaiter::Wait() {
  std::unique_lock lock(mutex_);
  if (counter_ != 0) {
    is_null_.wait(lock);
  }
}
}  // namespace tp
