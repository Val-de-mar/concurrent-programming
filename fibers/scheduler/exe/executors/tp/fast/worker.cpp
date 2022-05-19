#include <exe/executors/tp/fast/worker.hpp>
#include <exe/executors/tp/fast/thread_pool.hpp>

#include <twist/util/thread_local.hpp>

namespace exe::executors::tp::fast {

twist::util::ThreadLocalPtr<Worker> current_worker;

Worker::Worker(ThreadPool& host, size_t index)
    : host_(host) /*, index_(index)*/, impudence_(Active), metrics_(index) {
  host_.coordinator_.AcquireUnsafe(*this);
}

void Worker::Start() {
  thread_.emplace([this]() {
    Work();
  });
}

void Worker::Join() {
  thread_->join();
}

void Worker::PushToLocalQueue(TaskBase* task) {
  if (stopped_.load()) {
    task->Discard();
    ++metrics_.discarded_;
    return;
  }
  if (!local_tasks_.TryPush(task)) {
    OffloadTasksToGlobalQueue(task);
  }
}

void Worker::PushToLifoSlot(TaskBase* task) {
  if (stopped_.load()) {
    task->Discard();
    ++metrics_.discarded_;
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
      host_.coordinator_.AwakeOne();
    }
    if (lifo_usage_ < kLifoLimit && lifo_slot_ != nullptr) {
      ++lifo_usage_;
      ++metrics_.from_lifo_;
      return std::exchange(lifo_slot_, nullptr);
    }
    lifo_usage_ = 0;

    TaskBase* ans;
    ans = TryPickNextTask();
    if (ans != nullptr) {
      return ans;
    } else {
      ans = GrabTasksFromGlobalQueue();
    }
    if (ans != nullptr) {
      return ans;
    }

    if (impudence_ == Active) {
      if ((ans = TryStealTasks((host_.workers_.size() + 1) / 2)) != nullptr) {
        return ans;
      }
    }

    if (lifo_slot_ != nullptr) {
      continue;
    }
    host_.coordinator_.TryParkMe(*this);
  }
  host_.coordinator_.Release(*this);
  if (lifo_slot_ != nullptr) {
    lifo_slot_->Discard();
    ++metrics_.discarded_;
  }
  while (TaskBase* ans = TryPickNextTask()) {
    ans->Discard();
    ++metrics_.discarded_;
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
      ++metrics_.executed_;
    } catch (...) {
      next->Discard();
      ++metrics_.discarded_;
    }
    current_worker = nullptr;
  }
}

TaskBase* Worker::TryPickNextTask() {
  return local_tasks_.TryPop();
}

TaskBase* Worker::GrabTasksFromGlobalQueue() {
  std::array<TaskBase*, Worker::kLocalQueueCapacity / 2> buffer;
  auto grabbed = host_.global_tasks_.Grab(buffer, host_.Size() /*, index_*/);
  if (grabbed == 0) {
    host_.coordinator_.AllowParking();

    grabbed = host_.global_tasks_.Grab(buffer, host_.Size() /*, index_*/);
    if (grabbed == 0) {
      return nullptr;
    }
  }
  metrics_.grabbed_from_global_ += grabbed;
  if (grabbed != 1) {
    local_tasks_.PushMany(std::span(buffer).subspan(1, grabbed - 1));
  }
  return buffer[0];
}

TaskBase* Worker::TryStealTasks(size_t series) {
  std::array<TaskBase*, kLocalQueueCapacity> buffer;
  for (size_t i = 0; i < series; ++i) {
    Worker& victim = host_.coordinator_.GiveVictim(twister_());
    if (&victim == this) {
      continue;
    }
    auto stolen = victim.StealTasks(buffer);
    if (stolen == 0) {
      continue;
    }
    metrics_.stolen_ += stolen;
    if (stolen != 1) {
      local_tasks_.PushMany(std::span(buffer).subspan(1, stolen - 1));
    }
    return buffer[0];
  }

  return nullptr;
}

size_t Worker::StealTasks(std::span<TaskBase*> out_buffer) {
  auto limit = std::min(local_tasks_.LowSize() / 2, out_buffer.size());
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
  metrics_.offloaded_ += grabbed;
  host_.global_tasks_.Offload(std::span(buffer).subspan(grabbed));
  host_.coordinator_.AwakeOne();
}
void Worker::MakeActive() {
  impudence_.store(Active);
}
void Worker::MakePassive() {
  impudence_.store(Passive);
}

ThreadPool* ThreadPool::Current() {
  if (Worker::Current() == nullptr) {
    return nullptr;
  }
  return &(Worker::Current()->Host());
}

}  // namespace exe::executors::tp::fast
