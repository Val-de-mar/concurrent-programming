//
// Created by val-de-mar on 09.03.2022.
//

#pragma once

#include <twist/stdlike/thread.hpp>
#include <twist/stdlike/mutex.hpp>
#include <twist/stdlike/condition_variable.hpp>

namespace exe::tp {
class ZeroWaiter {
 public:
  ZeroWaiter& operator--();
  ZeroWaiter& operator++();
  void Wait();

 private:
  uint32_t counter_ = 0;
  twist::stdlike::mutex mutex_;
  twist::stdlike::condition_variable is_null_;
};
}  // namespace exe::tp
