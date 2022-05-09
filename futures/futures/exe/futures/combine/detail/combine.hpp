#pragma once

#include <exe/futures/core/future.hpp>

#include <vector>

namespace exe::futures::detail {

// Generic Combine algorithm

template <typename Combinator, typename T, typename... Args>
auto Combine(std::vector<Future<T>> futures, Args&&... args) {
  auto combinator =
      std::make_shared<Combinator>(futures.size(), std::forward<Args>(args)...);

  auto f = combinator->MakeFuture();

  for (auto& future : futures) {
    Combinator::Acquire(std::move(future), combinator);
  }

  return std::move(f);
}

}  // namespace exe::futures::detail
