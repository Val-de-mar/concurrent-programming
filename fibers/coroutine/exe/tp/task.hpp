#pragma once

#include <wheels/support/function.hpp>
#include <variant>

namespace exe::tp {

// Intrusive tasks?
// using Task = wheels::UniqueFunction<void()>;
struct TaskBlock {
  virtual void operator()() = 0;
  virtual ~TaskBlock() = default;
};

struct Task {
 public:
  Task(TaskBlock* function) : function_(function) {
  }

  Task(Task&& other) = default;

  Task(wheels::UniqueFunction<void()> functor) : function_(std::move(functor)) {
  }

  template <class OtherFunction>
  Task(OtherFunction functor)
      : function_(wheels::UniqueFunction<void()>(std::move(functor))) {
  }

  void operator()() {
    if (TaskBlock** block = std::get_if<TaskBlock*>(&function_)) {
      (**block)();
    } else if (wheels::UniqueFunction<void()>* basic =
                   std::get_if<wheels::UniqueFunction<void()>>(&function_)) {
      (*basic)();
    } else {
      assert(false);
    }
  }

  ~Task() {
  }

 private:
  std::variant<TaskBlock*, wheels::UniqueFunction<void()>> function_;
  //  bool ownership_;
};

}  // namespace exe::tp
