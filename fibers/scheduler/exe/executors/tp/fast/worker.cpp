#include <exe/executors/tp/fast/worker.hpp>
#include <exe/executors/tp/fast/thread_pool.hpp>

#include <twist/util/thread_local.hpp>

namespace exe::executors::tp::fast {

twist::util::ThreadLocalPtr<Worker> current_worker;

Worker::Worker(ThreadPool& host, size_t index) : host_(host), index_(index) {
}

void Worker::Start() {
  thread_.emplace([this]() {
    Work();
    //    std::cout << "out "  + std::to_string(index_) + "\n";
  });
}

void Worker::Join() {
  thread_->join();
}

void Worker::PushToLocalQueue(TaskBase* task) {
  //  std::cout << "pushed\n";
  if (stopped_.load()) {
    task->Discard();
    return;
  }
  if (!local_tasks_.TryPush(task)) {
    //    std::cout << "offload\n";
    OffloadTasksToGlobalQueue(task);
  }
}

void Worker::PushToLifoSlot(TaskBase* task) {
  if (stopped_.load()) {
    task->Discard();
    return;
  }
  if (lifo_slot_ == nullptr) {
    lifo_slot_ = task;
  } else {
    PushToLocalQueue(std::exchange(lifo_slot_, task));
  }
}

TaskBase* Worker::PickNextTask() {
  // [Periodically] Global queue
  // 0) LIFO slot
  // 1) Local queue
  // 2) Global queue
  // 3) Work stealing
  // 4) Park worker
  while (!stopped_.load()) {
    ++cycle_;
    if (cycle_ >= 10) {
      cycle_ = 0;
      host_.global_tasks_.WakeOne();
    }
    //    std::cout << "roll\n";
    if (lifo_usage_ < kLifoLimit && lifo_slot_ != nullptr) {
      ++lifo_usage_;
      return std::exchange(lifo_slot_, nullptr);
    }
    lifo_usage_ = 0;

    TaskBase* ans;
    ans = TryPickNextTask();
    if (ans != nullptr) {
      //            std::cout << std::to_string(index_) + " taken next, local: "
      //            + std::to_string(local_tasks_.LowSize()) + "\n";
      return ans;
    } else {
      //      std::cout << std::to_string(index_) + " cant get local " +
      //      std::to_string(local_tasks_.LowSize()) + "\n";
    }
    ans = GrabTasksFromGlobalQueue();
    if (ans != nullptr) {
      //            std::cout << std::to_string(index_) + " taken global local:
      //            " + std::to_string(local_tasks_.LowSize()) + "\n";
      return ans;
    }

    if ((ans = TryStealTasks(1)) != nullptr) {
      //      std::cout << "stolen\n";
      //      std::cout << std::to_string(index_) + " taken stolen\n";
      return ans;
    }

    if (lifo_slot_ != nullptr) {
      continue;
    }
    host_.global_tasks_.TryParkMe();
  }
  if (lifo_slot_ != nullptr) {
    lifo_slot_->Discard();
  }
  while (TaskBase* ans = TryPickNextTask()) {
    ans->Discard();
  }
  return nullptr;
}
twist::stdlike::atomic<uint32_t> valll{0};

void Worker::Work() {
  while (TaskBase* next = PickNextTask()) {
    current_worker = this;
    try {
      next->Run();
      valll.fetch_add(1);
    } catch (...) {
      next->Discard();
    }
    current_worker = nullptr;
    //    std::cout << "executed " + std::to_string(valll.load()) + " by " +
    //    std::to_string(index_) + "\n";
  }
}

TaskBase* Worker::TryPickNextTask() {
  return local_tasks_.TryPop();
}

TaskBase* Worker::GrabTasksFromGlobalQueue() {
  std::array<TaskBase*, Worker::kLocalQueueCapacity / 2> buffer;
  auto grabbed = host_.global_tasks_.Grab(buffer, host_.Size() /*, index_*/);
  if (grabbed == 0) {
    return nullptr;
  }
  //  for (size_t i = 0; i < grabbed; ++i) {
  //    std::cout << "push buffer " + std::to_string(uint64_t(buffer[i])) +
  //    "\n";
  //  }
  if (grabbed != 1) {
    local_tasks_.PushMany(std::span(buffer).subspan(1, grabbed - 1));
  }
  return buffer[0];
}

TaskBase* Worker::TryStealTasks(size_t series) {
  std::array<TaskBase*, kLocalQueueCapacity> buffer;
  for (size_t i = 0; i < series; ++i) {
    for (Worker& victim : host_.workers_) {
      if (&victim == this) {
        continue;
      }
      auto stolen = victim.StealTasks(buffer);
      //      std::cout << "stolen from " + std::to_string(victim.index_) +
      //      std::to_string(stolen) + "\n";
      if (stolen == 0) {
        continue;
      }
      if (stolen != 1) {
        local_tasks_.PushMany(std::span(buffer).subspan(1, stolen - 1));
      }
      return buffer[0];
    }
  }
  return nullptr;
}

size_t Worker::StealTasks(std::span<TaskBase*> out_buffer) {
  auto limit = std::min(local_tasks_.LowSize() / 2, out_buffer.size());
  //  std::cout <<  std::to_string(out_buffer.size()) + "limit " +
  //  std::to_string(limit) + "\n";
  return local_tasks_.Grab(out_buffer.subspan(0, limit));
}
Worker* Worker::Current() {
  return current_worker;
}
void Worker::OffloadTasksToGlobalQueue(TaskBase* overflow) {
  std::array<TaskBase*, kLocalQueueCapacity / 2 + 1> buffer;
  buffer[0] = overflow;
  auto grabbed = local_tasks_.Grab(std::span(buffer).subspan(1));
  ++grabbed;
  host_.global_tasks_.Offload(std::span(buffer).subspan(grabbed));
}

ThreadPool* ThreadPool::Current() {
  if (Worker::Current() == nullptr) {
    return nullptr;
  }
  return &(Worker::Current()->Host());
}

}  // namespace exe::executors::tp::fast
