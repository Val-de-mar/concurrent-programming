#pragma once

#include <exe/executors/task.hpp>
#include <exe/support/zero_waiter.hpp>

#include <twist/stdlike/atomic.hpp>
#include <twist/stdlike/mutex.hpp>

#include <wheels/intrusive/forward_list.hpp>
#include <wheels/support/defer.hpp>

#include <span>
#include <deque>

namespace exe::executors::tp::fast {

// Unbounded queue shared between workers

class GlobalQueue {
 public:
  twist::stdlike::atomic<uint32_t> ttt{1};
  void PushOne(TaskBase* item) {
    if (is_closed_) {
      item->Discard();
      return;
    }
    std::lock_guard lock(access_);
    if (queue_.empty()) {
      waiter_.IncrementValue();
    }
    queue_.push_back(item);
    //    std::cout << "push "  + std::to_string(ttt.fetch_add(1)) +" "+
    //    std::to_string(queue_.size()) + "\n";
    sleeper_.notify_one();
  }

  void Offload(std::span<TaskBase*> buffer) {
    //    std::cout << "offloaded " + std::to_string(buffer.size()) + "\n";
    if (is_closed_) {
      for (auto item : buffer) {
        item->Discard();
      }
      return;
    }
    std::lock_guard lock(access_);
    if (queue_.empty() && !buffer.empty()) {
      waiter_.IncrementValue();
    }
    for (auto task : buffer) {
      queue_.push_back(task);
    }
    sleeper_.notify_all();
  }

  // Returns nullptr if queue is empty
  TaskBase* TryPopOne() {
    std::lock_guard lock(access_);
    //    std::cout << "pop\n";
    if (queue_.empty()) {
      return nullptr;
    }
    if (queue_.size() == 1) {
      waiter_.DecrementValue();
    }
    wheels::Defer pop([this]() {
      queue_.pop_front();
    });
    return (TaskBase*)(queue_.front());
  }

  // Returns number of items in `out_buffer`
  size_t Grab(std::span<TaskBase*> out_buffer, size_t workers /*, size_t by*/) {
    std::lock_guard lock(access_);
    auto limit = std::min(
        std::min(out_buffer.size(), queue_.size()),
        queue_.size() / workers + (queue_.size() % workers != 0 ? 1 : 0));

    if (limit == 0) {
      return 0;
    }

    //    std::cout << std::to_string(limit) + "\n" << std::endl;

    if (limit == queue_.size()) {
      waiter_.DecrementValue();
    }
    for (size_t i = 0; i < limit; ++i) {
      out_buffer[i] = queue_.front();
      queue_.pop_front();
    }
    return limit;
  }

  void TryParkMe() {
    std::unique_lock lock(access_);
    while (queue_.empty() && !is_closed_.load() && !helper_request_.load()) {
      waiter_.DecrementValue();
      //      std::cout << "sleep" +
      //      std::to_string(int(std::this_thread::get_id())) + "\n";
      sleeper_.wait(lock);
      waiter_.IncrementValue();
      //      std::cout << "awake" + std::to_string(index) + "\n";
    }
    helper_request_.store(false);
  }

  void WakeOne() {
    helper_request_.store(true);
    sleeper_.notify_one();
  }

  void WaitIdle() {
    waiter_.Wait();
  }
  void Stop() {
    //    std::cout << "рота подъем\n";
    is_closed_.store(true);
    {
      std::unique_lock lock(access_);
      sleeper_.notify_all();
    }
    std::array<TaskBase*, 256> buf;
    while (true) {
      size_t i = 0;
      {
        std::lock_guard lock(access_);
        for (; i < buf.size() && !queue_.empty(); ++i) {
          buf[i] = queue_.front();
          queue_.pop_front();
        }
        if (i == 0) {
          break;
        }
      }
      for (size_t j = 0; j < i; ++j) {
        buf[j]->Discard();
      }
    }
  }

  explicit GlobalQueue(size_t workers) : access_(), queue_(), waiter_(workers) {
  }

 private:
  twist::stdlike::mutex access_;
  std::deque<TaskBase*> queue_;
  twist::stdlike::atomic<bool> is_closed_{false};
  twist::stdlike::atomic<bool> helper_request_{false};
  twist::stdlike::condition_variable sleeper_;
  support::ZeroWaiter waiter_;
};

}  // namespace exe::executors::tp::fast
