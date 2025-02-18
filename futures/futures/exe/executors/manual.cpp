#include <exe/executors/manual.hpp>

namespace exe::executors {

void ManualExecutor::Execute(Task task) {
  tasks_.push(std::move(task));
}

// Run tasks

size_t ManualExecutor::RunAtMost(size_t limit) {
  size_t completed = 0;
  for (; completed < limit && HasTasks(); ++completed) {
    tasks_.front()->Run();
    tasks_.pop();
  }
  return completed;
}

size_t ManualExecutor::Drain() {
  size_t completed = 0;
  while (HasTasks()) {
    tasks_.front()->Run();
    tasks_.pop();
    ++completed;
  }
  return completed;
}

}  // namespace exe::executors