#pragma once

#include <exe/executors/executor.hpp>

namespace exe::executors {

/*
 * Usage:
 * Execute(thread_pool, []() {
 *   std::cout << "Hi" << std::endl;
 * });
 */

template <typename F>
void Execute(IExecutor& where, F&& f) {
  where.Execute(HeapFunctionTask<std::remove_reference_t<F>>::CreateNew(
      std::forward<F>(f)));
}

}  // namespace exe::executors
