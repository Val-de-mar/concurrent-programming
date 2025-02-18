#include <exe/executors/strand.hpp>

namespace exe::executors {

Strand::Strand(IExecutor& underlying) : slave_(underlying) {
}

void Strand::Execute(Task task) {
  auto created = new TaskNode{.task_ = std::move(task)};
  while (true) {
    created->prev_ = tasks_;
    auto expected = created->prev_;
    if (tasks_.compare_exchange_strong(expected, created)) {
      break;
    }
  }
  if (reschedule_.exchange(false)) {
    Schedule();
  }
}

void Strand::Schedule() {
  slave_.Execute(static_cast<TaskBase*>(this));
}

void Strand::Run() {
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
    stolen->task_->Run();
    stolen->task_->Discard();
    auto save = stolen->prev_;
    delete stolen;
    stolen = save;
  }
  Schedule();
}
void Strand::Discard() noexcept {
}

Strand::TaskNode* Strand::TaskNode::Reverse() {
  TaskNode* cur = nullptr;
  TaskNode* prev = this;

  while (prev != nullptr) {
    TaskNode* save = prev->prev_;
    prev->prev_ = cur;
    cur = prev;
    prev = save;
  }
  return cur;
}

}  // namespace exe::executors
