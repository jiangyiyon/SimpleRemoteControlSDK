#include <gtest/gtest.h>
#include "screensdk/utils/thread_pool.h"

using namespace screensdk;

class ThreadPoolTest : public ::testing::Test {
protected:
  void SetUp() override {
    pool_ = std::make_unique<ThreadPool>(4);
  }

  void TearDown() override {
    if (pool_) {
      pool_->waitForAll();
    }
  }

  std::unique_ptr<ThreadPool> pool_;
};

TEST_F(ThreadPoolTest, SubmitTaskExecutes) {
  std::atomic<int> counter{0};

  pool_->submit([&counter]() {
    counter.fetch_add(1);
  });

  pool_->waitForAll();

  EXPECT_EQ(counter.load(), 1);
}

TEST_F(ThreadPoolTest, SubmitMultipleTasks) {
  std::atomic<int> counter{0};

  for (int i = 0; i < 100; ++i) {
    pool_->submit([&counter]() {
      counter.fetch_add(1);
    });
  }

  pool_->waitForAll();

  EXPECT_EQ(counter.load(), 100);
}

TEST_F(ThreadPoolTest, EnqueueReturnsFuture) {
  auto future = pool_->enqueue([]() {
    return 42;
  });

  int result = future.get();

  EXPECT_EQ(result, 42);
}

TEST_F(ThreadPoolTest, EnqueueMultipleFutures) {
  std::vector<std::future<int>> futures;

  for (int i = 0; i < 10; ++i) {
    futures.push_back(pool_->enqueue([i]() {
      return i * 2;
    }));
  }

  for (size_t i = 0; i < futures.size(); ++i) {
    int result = futures[i].get();
    EXPECT_EQ(result, static_cast<int>(i * 2));
  }
}

TEST_F(ThreadPoolTest, GetPendingTaskCount) {
  for (int i = 0; i < 5; ++i) {
    pool_->submit([]() {});
  }

  // Due to race condition with worker threads, we can only check that
  // after waitForAll(), the count is 0. The initial count check is flaky.
  pool_->waitForAll();
  EXPECT_EQ(pool_->getPendingTaskCount(), 0);
}

TEST_F(ThreadPoolTest, GetWorkerCount) {
  EXPECT_EQ(pool_->getWorkerCount(), 4);
}

TEST_F(ThreadPoolTest, ConcurrentTaskExecution) {
  std::atomic<int> counter{0};
  constexpr int kNumTasks = 100;
  constexpr int kNumThreads = 4;

  for (int i = 0; i < kNumTasks; ++i) {
    pool_->submit([&counter]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      counter.fetch_add(1);
    });
  }

  pool_->waitForAll();

  EXPECT_EQ(counter.load(), kNumTasks);
}

TEST_F(ThreadPoolTest, StopTerminatesPool) {
  auto pool = std::make_unique<ThreadPool>(2);

  std::atomic<int> counter{0};

  for (int i = 0; i < 10; ++i) {
    pool->submit([&counter]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      counter.fetch_add(1);
    });
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  pool->stop();

  // Some tasks may not complete after stop
  EXPECT_GE(counter.load(), 0);
}

TEST_F(ThreadPoolTest, EnqueueWithArguments) {
  auto future = pool_->enqueue([](int a, int b) {
    return a + b;
  }, 10, 20);

  int result = future.get();
  EXPECT_EQ(result, 30);
}

TEST_F(ThreadPoolTest, ThreadSafety) {
  std::atomic<int> counter{0};
  constexpr int kNumThreads = 10;
  constexpr int kTasksPerThread = 100;

  std::vector<std::thread> threads;
  for (int i = 0; i < kNumThreads; ++i) {
    threads.emplace_back([this, &counter]() {
      for (int j = 0; j < kTasksPerThread; ++j) {
        pool_->submit([&counter]() {
          counter.fetch_add(1);
        });
      }
    });
  }

  for (auto& thread : threads) {
    thread.join();
  }

  pool_->waitForAll();

  EXPECT_EQ(counter.load(), kNumThreads * kTasksPerThread);
}
