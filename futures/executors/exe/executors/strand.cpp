#include <exe/executors/strand.hpp>

namespace exe::executors {

Strand::Strand(IExecutor& underlying) : slave_(underlying) {
}
void Strand::Execute(Task task) {
  auto created = new TaskNode{.task_ = std::move(task)};
  while (true) {
    created->prev_.store(tasks_);
    auto expected = created->prev_.load();
    if (tasks_.compare_exchange_strong(expected, created)) {
      break;
    }
  }
  if (reschedule_.exchange(false)) {
    Schedule();
  }
}

void Strand::Schedule() {
  slave_.Execute([this]() {
    auto stolen = tasks_.exchange(nullptr);
    if (stolen == nullptr) {
      reschedule_.store(true);
      if (tasks_.load() == nullptr) {
        return;
      }
      if (!reschedule_.exchange(false)) {
        return;
      }
      Schedule();
      return;
    }
    stolen = stolen->Reverse();
    while (stolen != nullptr) {
      stolen->task_();
      auto save = stolen->prev_.load();
      delete stolen;
      stolen = save;
    }

    if (tasks_.load() == nullptr) {
      reschedule_.store(true);
      if (tasks_.load() == nullptr) {
        return;
      }
      if (!reschedule_.exchange(false)) {
        return;
      }
      Schedule();
      return;
    }
    Schedule();
  });
}

Strand::TaskNode* Strand::TaskNode::Reverse() {
  TaskNode* cur = nullptr;
  TaskNode* prev = this;

  while (prev != nullptr) {
    TaskNode* save = prev->prev_.load();
    prev->prev_.store(cur);
    cur = prev;
    prev = save;
  }
  return cur;
}
}  // namespace exe::executors