#pragma once

#include <exe/futures/core/future.hpp>

namespace exe::futures {

template <typename T>
Future<T> JustValue(T value) {
  auto [f, p] = MakeContract<T>();
  std::move(p).SetValue(std::move(value));
  return {std::move(f)};
}

}  // namespace exe::futures
