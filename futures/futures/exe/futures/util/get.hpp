#pragma once

#include <exe/futures/core/future.hpp>
#include <exe/executors/inline.hpp>
#include <twist/stdlike/atomic.hpp>

namespace exe::futures {

// ~ std::future::get
// Blocking
template <typename T>
wheels::Result<T> GetResult(Future<T> future) {
  twist::stdlike::atomic<uint32_t> sleeper{0};
  std::optional<wheels::Result<T>> ans;

  std::move(future)
      .Via(executors::GetInlineExecutor())
      .Subscribe([&ans, &sleeper](auto result) {
        ans = std::move(result);
        sleeper.store(1);
        sleeper.FutexWakeAll();
      });

  while (sleeper.load() == 0) {
    sleeper.FutexWait(0);
  }

  return {std::move(*ans)};
}

// Blocking
template <typename T>
T GetValue(Future<T> future) {
  return GetResult(std::move(future)).ValueOrThrow();
}

}  // namespace exe::futures
