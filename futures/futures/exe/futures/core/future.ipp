#ifndef FUTURE_IMPL
#error Do not include this file directly
#endif

#include <exe/futures/core/detail/traits.hpp>

namespace exe::futures {

//////////////////////////////////////////////////////////////////////

// Static constructors

template <typename T>
Future<T> Future<T>::Invalid() {
  return Future<T>{nullptr};
}

//////////////////////////////////////////////////////////////////////

// Executors
template <class T>
void see(T&);

template <typename T>
Future<T> Future<T>::Via(executors::IExecutor& e) && {
  auto state = ReleaseState();
  state->SetExecutor(&e);
  return Future{std::move(state)};
}

template <typename T>
executors::IExecutor& Future<T>::GetExecutor() const {
  return AccessState().GetExecutor();
}

template <typename T>
void Future<T>::Subscribe(Callback<T> callback) && {
  return ReleaseState()->SetCallback(std::move(callback));
}

//////////////////////////////////////////////////////////////////////

// Synchronous Then

template <typename T>
template <typename F>
  requires SyncContinuation<F, T>
Future<std::invoke_result_t<F, T>> Future<T>::Then(F continuation) && {
  using U = std::invoke_result_t<F, T>;

  auto [f, p] = MakeContractVia<U>(GetExecutor());

  ReleaseState()->SetCallback(
      [p = std::move(p), continuation = std::move(continuation)](
          wheels::Result<T> result) mutable {
        if (result.HasError()) {
          std::move(p).SetError(result.GetError());
          return;
        }
        try {
          std::move(p).SetValue(continuation(std::move(result).ExpectValue()));
        } catch (...) {
          std::move(p).SetError(std::current_exception());
        }
      });

  return Future<U>{std::move(f)};
}

//////////////////////////////////////////////////////////////////////

// Asynchronous Then

template <typename T>
template <typename F>
  requires AsyncContinuation<F, T>
Future<typename detail::Flatten<std::invoke_result_t<F, T>>::ValueType>
Future<T>::Then(F continuation) && {
  using U = typename detail::Flatten<std::invoke_result_t<F, T>>::ValueType;

  auto [f, p] = MakeContractVia<U>(GetExecutor());

  ReleaseState()->SetCallback(
      [p = std::move(p), continuation = std::move(continuation)](
          wheels::Result<T> result) mutable {
        if (result.HasError()) {
          std::move(p).SetError(result.GetError());
          return;
        }
        try {
          Future<U> future = continuation(std::move(result.ExpectValue()));
          std::move(future).Subscribe([p = std::move(p)](auto res) mutable {
            std::move(p).Set(std::move(res));
          });
        } catch (...) {
          std::move(p).SetError(std::current_exception());
        }
      });
  return std::move(f);
}

//////////////////////////////////////////////////////////////////////

// Recover

template <typename T>
template <typename F>
  requires ErrorHandler<F, T>
Future<T> Future<T>::Recover(F error_handler) && {
  Contract<T> contract = MakeContractVia<T>(GetExecutor());
  ReleaseState()->SetCallback(
      [handler = std::move(error_handler),
       p = std::move(contract.promise)](wheels::Result<T> result) mutable {
        if (result.HasValue()) {
          std::move(p).SetError(result.GetError());
          return;
        }
        try {
          std::move(p).SetValue(handler(std::move(result).GetError()));
        } catch (...) {
          std::move(p).SetError(std::current_exception());
        }
        return;
      });
  return std::move(contract.future);
}

}  // namespace exe::futures
