#pragma once

#include <exe/futures/core/future.hpp>
#include <exe/futures/combine/detail/combine.hpp>
#include <exe/futures/util/just.hpp>

#include <wheels/support/vector.hpp>

namespace exe::futures {

namespace detail {

template <typename T>
class AllCombinator {
 public:
  explicit AllCombinator(size_t inputs)
      : ans_(inputs, std::nullopt), limit_(inputs) {
  }

  static void Acquire(Future<T> future, std::shared_ptr<AllCombinator> self) {
    auto number = self->acquired_;
    ++(self->acquired_);
    std::move(future).Subscribe(
        [self = std::move(self), number](auto res) mutable {
          self->Release(std::move(res), number);
        });
  }

  void Release(wheels::Result<T> res, size_t number) {
    if (res.HasError()) {
      if (!exited_.exchange(true)) {
        std::move(*promise_).SetError(res.GetError());
      }
      return;
    }
    ans_[number] = std::move(res).ExpectValue();
    if (finished_.fetch_add(1) + 1 == limit_) {
      std::vector<T> prepare;
      for (auto& val : ans_) {
        prepare.push_back(std::move(*val));
      }
      std::move(*promise_).SetValue(std::move(prepare));
    }
  }

  auto MakeFuture() {
    auto [f, p] = MakeContract<std::vector<T>>();
    promise_ = std::move(p);
    return Future{std::move(f)};
  }

 private:
  std::optional<Promise<std::vector<T>>> promise_;

  std::vector<std::optional<T>> ans_;

  twist::stdlike::atomic<uint64_t> finished_{0};
  twist::stdlike::atomic<bool> exited_{false};
  size_t acquired_{0};
  const size_t limit_;
};

}  // namespace detail

// All combinator
// All values or first error
// std::vector<Future<T>> -> Future<std::vector<T>>

template <typename T>
Future<std::vector<T>> All(std::vector<Future<T>> inputs) {
  if (inputs.empty()) {
    return JustValue<std::vector<T>>({});
  }
  return detail::Combine<detail::AllCombinator<T>>(std::move(inputs));
}

// Usage:
// auto all = futures::All(std::move(f1), std::move(f2));

template <typename T, typename... Fs>
Future<std::vector<T>> All(Future<T>&& first, Fs&&... rest) {
  return All(wheels::ToVector(std::move(first), std::forward<Fs>(rest)...));
}

}  // namespace exe::futures
