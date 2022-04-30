#pragma once

#include <exe/tp/thread_pool.hpp>

namespace exe::tp {

template <typename F>
void Submit(ThreadPool& thread_pool, F&& f) {
  thread_pool.Submit(HeapFunctionTask<std::remove_reference_t<F>>::CreateNew(
      std::forward<F>(f)));
}

}  // namespace exe::tp
