#pragma once

#include <exe/futures/core/callback.hpp>

#include <exe/executors/executor.hpp>
#include <exe/executors/execute.hpp>

#include <twist/stdlike/atomic.hpp>

#include <wheels/result/result.hpp>
#include <wheels/support/function.hpp>

#include <optional>
#include <memory>

namespace exe::futures {

namespace detail {

//////////////////////////////////////////////////////////////////////

// State shared between Promise and Future

template <typename T>
class SharedState {
 public:
  explicit SharedState(executors::IExecutor* executor) : executor_(executor) {
  }

  bool HasResult() const {
    return result_ready_.load() >= 1;
  }

  wheels::Result<T> GetReadyResult() {
    while (result_ready_.load() == 0) {
      result_ready_.FutexWait(0);
    }
    return std::move(*result_);
  }

  void SetExecutor(executors::IExecutor* executor) {
    executor_ = executor;
  }

  executors::IExecutor& GetExecutor() const {
    return *executor_;
  }

  // Producer
  // Progress guarantee: wait-free
  void SetResult(wheels::Result<T> result) {
    result_ = {std::move(result)};
    result_ready_.store(1);
    TryCallback();
    result_ready_.notify_all();
  }

  // Consumer
  // Progress guarantee: wait-free
  void SetCallback(Callback<T> callback) {
    callback_ = std::move(callback);
    callback_ready_.store(true);
    if (result_ready_.load() == 1) {
      TryCallback();
    }
  }

 private:
  void TryCallback() {
    if (callback_ready_.exchange(false)) {
      Execute(*executor_, [callback = std::move(callback_),
                           val = std::move(*result_)]() mutable {
        callback(std::move(val));
      });
    }
  }

  std::optional<wheels::Result<T>> result_;
  Callback<T> callback_;
  executors::IExecutor* executor_;
  twist::stdlike::atomic<uint32_t> result_ready_{0};
  twist::stdlike::atomic<bool> callback_ready_{false};
};

//////////////////////////////////////////////////////////////////////

template <typename T>
using StateRef = std::shared_ptr<SharedState<T>>;

template <typename T>
StateRef<T> MakeSharedState(executors::IExecutor& executor) {
  return std::make_shared<SharedState<T>>(&executor);
}

//////////////////////////////////////////////////////////////////////

// Common base for Promise and Future

template <typename T>
class HoldState {
  using State = SharedState<T>;

 protected:
  explicit HoldState(StateRef<T> state) : state_(std::move(state)) {
  }

  // Movable
  HoldState(HoldState&& that) = default;
  HoldState& operator=(HoldState&& that) = default;

  // Non-copyable
  HoldState(const HoldState& that) = delete;
  HoldState& operator=(const HoldState& that) = delete;

  StateRef<T> ReleaseState() {
    CheckState();
    return std::move(state_);
  }

  bool HasState() const {
    return (bool)state_;
  }

  const State& AccessState() const {
    CheckState();
    return *state_.get();
  }

 private:
  void CheckState() const {
    WHEELS_VERIFY(HasState(), "No shared state / shared state released");
  }

 protected:
  StateRef<T> state_;
};

}  // namespace detail

}  // namespace exe::futures
