#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace screensdk {

/**
 * @brief Task type for thread pool
 */
using Task = std::function<void()>;

/**
 * @brief Thread pool for async task execution
 * 
 * Provides efficient thread pool with:
 * - Fixed number of worker threads
 * - Task queue with priority support (simplified)
 * - Graceful shutdown
 * 
 * T014: Implement thread pool for async task execution
 */
class ThreadPool {
public:
  /**
   * @brief Create thread pool with specified number of worker threads
   * @param num_threads Number of worker threads (0 = hardware concurrency)
   */
  explicit ThreadPool(size_t num_threads = 0);

  /**
   * @brief Destructor - waits for all tasks to complete
   */
  ~ThreadPool();

  // Disable copy and move
  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;
  ThreadPool(ThreadPool&&) = delete;
  ThreadPool& operator=(ThreadPool&&) = delete;

  /**
   * @brief Submit task for execution
   * @param task Task to execute
   */
  void submit(Task task);

  /**
   * @brief Submit task and return future for result
   * @tparam F Function type
   * @tparam Args Argument types
   * @param f Function to execute
   * @param args Arguments for the function
   * @return Future for the result
   */
  template<typename F, typename... Args>
  auto enqueue(F&& f, Args&&... args)
      -> std::future<typename std::invoke_result<F, Args...>::type> {
    using return_type = typename std::invoke_result<F, Args...>::type;

    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));

    std::future<return_type> res = task->get_future();
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      if (stop_) {
        throw std::runtime_error("enqueue on stopped ThreadPool");
      }
      tasks_.emplace([task](){ (*task)(); });
    }
    condition_.notify_one();
    return res;
  }

  /**
   * @brief Wait for all queued tasks to complete
   */
  void waitForAll();

  /**
   * @brief Get number of pending tasks
   */
  size_t getPendingTaskCount() const;

  /**
   * @brief Get number of worker threads
   */
  size_t getWorkerCount() const { return workers_.size(); }

  /**
   * @brief Stop the thread pool (immediate)
   */
  void stop();

private:
  /**
   * @brief Worker thread function
   */
  void worker();

  // Worker threads
  std::vector<std::thread> workers_;

  // Task queue
  std::queue<Task> tasks_;

  // Synchronization
  mutable std::mutex queue_mutex_;
  std::condition_variable condition_;
  std::atomic<bool> stop_{false};
  std::atomic<size_t> active_tasks_{0};  // Number of currently executing tasks
};

} // namespace screensdk
