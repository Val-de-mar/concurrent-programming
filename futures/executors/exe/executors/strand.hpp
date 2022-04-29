#pragma once

#include <exe/executors/executor.hpp>
#include <twist/stdlike/atomic.hpp>

#include <optional>

namespace exe::executors {

// Strand (serial executor, asynchronous mutex)
// Executes (via underlying executor) tasks
// non-concurrently and in FIFO order

/*template <typename T>
class LockFreeMPSCStack {
  struct Node {
    Node* prev_;
    T data_;
  };

 public:

  void Push(T value) {
    auto created = new Node{.prev_ = {}, .data_ = std::move(value)};
    while (true) {
      created->prev_ = head_.load(std::memory_order_relaxed);
      auto expected = created->prev_.load();
      if (head_.compare_exchange_weak(expected, created)) {
        break;
      }
    }
  }


  std::optional<T> Pop() {
    if (ready_to_consume_ == nullptr) {
      ready_to_consume_ = head_.exchange(nullptr);
    }
    T ans;
    return ans;
  }

 private:

  static Node* Reverse(Node* head) {
    Node* cur = nullptr;
    Node* prev = head;

    while (prev != nullptr) {
      TaskNode* save = prev->prev_.load();
      prev->prev_.store(cur);
      cur = prev;
      prev = save;
    }
    return cur;
  }


  twist::stdlike::atomic<Node*> head_;
  Node* ready_to_consume_;
};*/

class Strand : public IExecutor, public TaskBase {
  struct TaskNode {
    TaskNode* prev_;
    Task task_;
    TaskNode* Reverse();
  };

  void Schedule();

 public:
  explicit Strand(IExecutor& underlying);

  // IExecutor
  void Execute(Task task) override;

  virtual ~Strand() = default;

  void Run() override;
  void Discard() noexcept override;

 private:
  IExecutor& slave_;
  twist::stdlike::atomic<TaskNode*> tasks_{nullptr};
  twist::stdlike::atomic<bool> reschedule_{true};
};

}  // namespace exe::executors
