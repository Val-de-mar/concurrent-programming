#pragma once

#include <wheels/support/function.hpp>
#include <wheels/intrusive/forward_list.hpp>

namespace exe::tp {

struct ITask {
  virtual ~ITask() = default;

  virtual void Run() = 0;
  virtual void Discard() noexcept = 0;
};

// Intrusive task
struct TaskBase : ITask, wheels::IntrusiveForwardListNode<TaskBase> {
  ~TaskBase() override = default;
};

template <typename FunctionT>
struct HeapFunctionTask : TaskBase {
  ~HeapFunctionTask() override = default;

  void Run() override {
    function_();
    delete this;
  }

  void Discard() noexcept override {
    delete this;
  }

  static HeapFunctionTask* CreateNew(FunctionT function) {
    return new HeapFunctionTask<FunctionT>(std::move(function));
  }

 private:
  explicit HeapFunctionTask(FunctionT function)
      : function_(std::move(function)) {
  }

 private:
  FunctionT function_;
};

using Task = TaskBase*;

}  // namespace exe::tp
