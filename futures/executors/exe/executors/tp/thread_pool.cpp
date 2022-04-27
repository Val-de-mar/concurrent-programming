#include <exe/executors/tp/thread_pool.hpp>

#include <twist/util/thread_local.hpp>

namespace exe::executors {

////////////////////////////////////////////////////////////////////////////////

static twist::util::ThreadLocalPtr<ThreadPool> pool;

////////////////////////////////////////////////////////////////////////////////

ThreadPool::ThreadPool(size_t /*workers*/) {
}

ThreadPool::~ThreadPool() {
}

void ThreadPool::Execute(Task /*task*/) {
}

void ThreadPool::WaitIdle() {
}

void ThreadPool::Stop() {
}

ThreadPool* ThreadPool::Current() {
  return pool;
}

}  // namespace exe::executors
