#include <exe/executors/tp/fast/coordinator.hpp>

namespace exe::executors::tp::fast {

//

void Coordinator::TryParkMe() {
  while (unfinished_.load() == 0) {
    active_.fetch_sub(1);
    unfinished_.FutexWait(0);
    active_.fetch_add(1);
  }
}
}  // namespace exe::executors::tp::fast
