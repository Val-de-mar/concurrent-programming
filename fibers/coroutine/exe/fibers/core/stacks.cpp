#include <exe/fibers/core/stacks.hpp>
#include <wheels/support/defer.hpp>
#include <vector>
#include <twist/stdlike/mutex.hpp>

using context::Stack;

namespace exe::fibers {

//////////////////////////////////////////////////////////////////////

class StackAllocator {
 public:
  Stack Allocate() {
    std::unique_lock lock(mutex_);
    if (freed_.empty()) {
      lock.unlock();
      return AllocateNewStack();
    } else {
      wheels::Defer pop([&vec = this->freed_]() {
        vec.pop_back();
      });
      return std::move(freed_.back());
    }
  }

  void Release(Stack stack) {
    if (stack.Size() == 0) {
      return;
    }
    std::lock_guard lock(mutex_);
    freed_.push_back(std::move(stack));
  }

 private:
  static Stack AllocateNewStack() {
    static const size_t kStackPages = 16;  // 16 * 4KB = 64KB
    return Stack::AllocatePages(kStackPages);
  }

 private:
  // Pool
  std::vector<context::Stack> freed_;
  twist::stdlike::mutex mutex_;
};

//////////////////////////////////////////////////////////////////////

StackAllocator allocator;

context::Stack AllocateStack() {
  return allocator.Allocate();
}

void ReleaseStack(context::Stack stack) {
  allocator.Release(std::move(stack));
}

}  // namespace exe::fibers
