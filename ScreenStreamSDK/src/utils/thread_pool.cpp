#include "screensdk/utils/thread_pool.h"

namespace screensdk {

ThreadPool::ThreadPool(size_t num_threads) {
  if (num_threads == 0) {
    num_threads = std::thread::hardware_concurrency();
    if (num_threads == 0) {
      num_threads = 4;  // Default fallback
    }
  }

  for (size_t i = 0; i < num_threads; ++i) {
    workers_.emplace_back(&ThreadPool::worker, this);
  }
}

ThreadPool::~ThreadPool() {
  stop();
  for (auto& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

void ThreadPool::submit(Task task) {
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    if (stop_) {
      throw std::runtime_error("submit on stopped ThreadPool");
    }
    tasks_.push(std::move(task));
  }
  condition_.notify_one();
}

void ThreadPool::waitForAll() {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  condition_.wait(lock, [this]() {
    return tasks_.empty() && active_tasks_ == 0 || stop_;
  });
}

size_t ThreadPool::getPendingTaskCount() const {
  std::unique_lock<std::mutex> lock(queue_mutex_);
  return tasks_.size();
}

void ThreadPool::stop() {
  {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    stop_ = true;
  }
  condition_.notify_all();
}

void ThreadPool::worker() {
  while (true) {
    Task task;

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      condition_.wait(lock, [this]() {
        return stop_ || !tasks_.empty();
      });

      if (stop_ && tasks_.empty()) {
        return;
      }

      task = std::move(tasks_.front());
      tasks_.pop();
      active_tasks_++;
    }

    task();
    active_tasks_--;
    condition_.notify_all();
  }
}

} // namespace screensdk
