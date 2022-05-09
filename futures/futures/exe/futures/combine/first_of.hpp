#pragma once

#include <exe/futures/core/future.hpp>
#include <exe/futures/combine/detail/combine.hpp>

#include <wheels/support/vector.hpp>

namespace exe::futures {

//////////////////////////////////////////////////////////////////////

namespace detail {

template <typename T>
class FirstOfCombinator {
 public:
  Future<T> MakeFuture() {
    auto [f, p] = MakeContract<T>();
    promise_ = std::move(p);
    return {std::move(f)};
  }
  explicit FirstOfCombinator(size_t inputs) : errors_left_(inputs) {
  }

  static void Acquire(Future<T> future,
                      std::shared_ptr<FirstOfCombinator> self) {
    std::move(future).Subscribe([self = std::move(self)](auto res) mutable {
      self->Release(std::move(res));
    });
  }

  void Release(wheels::Result<T> res) {
    if (res.HasError()) {
      if (errors_left_.fetch_sub(1) - 1 == 0) {
        std::move(*promise_).Set(std::move(res));
      }
      return;
    }

    if (nobody_is_ready_.exchange(false)) {
      std::move(*promise_).Set(std::move(res));
    }
  }

 private:
 private:
  std::optional<Promise<T>> promise_;
  twist::stdlike::atomic<bool> nobody_is_ready_{true};
  twist::stdlike::atomic<uint32_t> errors_left_{};
};

}  // namespace detail

// FirstOf combinator
// First value or last error
// std::vector<Future<T>> -> Future<T>

template <typename T>
Future<T> FirstOf(std::vector<Future<T>> inputs) {
  return detail::Combine<detail::FirstOfCombinator<T>>(std::move(inputs));
}

// Usage:
// auto first_of = futures::FirstOf(std::move(f1), std::move(f2));

template <typename T, typename... Fs>
auto FirstOf(Future<T>&& first, Fs&&... rest) {
  return FirstOf(wheels::ToVector(std::move(first), std::forward<Fs>(rest)...));
}

}  // namespace exe::futures
