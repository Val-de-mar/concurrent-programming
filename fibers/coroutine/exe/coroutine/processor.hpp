#pragma once

#include <exe/coroutine/impl.hpp>
#include <context/stack.hpp>

#include <optional>

namespace exe::coroutine::processors {

template <typename T>
class Processor {
 public:
  explicit Processor(Routine /*routine*/) {
    // Not implemented
  }

  void Send(T /*value*/) {
    // Not implemented
  }

  void Close() {
    // Not implemented
  }

  static std::optional<T> Receive() {
    return std::nullopt;
  }

 private:
  static context::Stack AllocateStack() {
    static const size_t kStackPages = 16;  // 16 * 4KB = 64KB
    return context::Stack::AllocatePages(kStackPages);
  }

 private:
  // ???
};

// Shortcut
template <typename T>
std::optional<T> Receive() {
  return Processor<T>::Receive();
}

}  // namespace exe::coroutine::processors
