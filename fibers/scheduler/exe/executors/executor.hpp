#pragma once

#include <exe/executors/task.hpp>

namespace exe::executors {

enum class Hint {
  UpToYou = 1,  // Rely on executor scheduling decision
  Next = 2      // Use LIFO scheduling
};

struct IExecutor {
  virtual ~IExecutor() = default;

  // TODO: Support hints
  virtual void Execute(TaskBase* task, Hint hint = Hint::UpToYou) = 0;
};

}  // namespace exe::executors
