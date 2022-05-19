#pragma once

#include <exe/executors/executor.hpp>

namespace exe::executors {

template <typename F>
void Execute(IExecutor& where, F&& f) {
  where.Execute(HeapFunctionTask<std::remove_reference_t<F>>::CreateNew(
                    std::forward<F>(f)),
                Hint::UpToYou);
}

}  // namespace exe::executors
