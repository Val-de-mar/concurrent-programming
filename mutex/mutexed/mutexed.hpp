#pragma once

#include <twist/stdlike/mutex.hpp>
#include <type_traits>

namespace util {

//////////////////////////////////////////////////////////////////////

// Safe API for mutual exclusion

template <typename T>
class Mutexed {
  using MutexImpl = twist::stdlike::mutex;

  class UniqueRef {
    std::lock_guard<MutexImpl> guard_;
    T& object_;

    UniqueRef(MutexImpl& mutex, T& object) : guard_(mutex), object_(object){};

   public:
    // Non-copyable
    UniqueRef(const UniqueRef&) = delete;

    // Non-movable
    UniqueRef(UniqueRef&&) = delete;

    // operator*
    std::add_lvalue_reference_t<T> operator*() const {
      return object_;
    }

    // operator->
    std::remove_reference_t<T>* operator->() const {
      return &object_;
    }

    friend class Mutexed<T>;
  };

 public:
  // https://eli.thegreenplace.net/2014/perfect-forwarding-and-universal-references-in-c/
  template <typename... Args>
  explicit Mutexed(Args&&... args) : object_(std::forward<Args>(args)...) {
  }

  UniqueRef Lock() {
    return {mutex_, object_};
  }

 private:
  T object_;
  MutexImpl mutex_;  // Guards access to object_
};

//////////////////////////////////////////////////////////////////////

// Helper function for single operations over shared object:
// Usage:
//   Mutexed<vector<int>> ints;
//   Locked(ints)->push_back(42);

template <typename T>
auto Locked(Mutexed<T>& object) {
  return object.Lock();
}

}  // namespace util
