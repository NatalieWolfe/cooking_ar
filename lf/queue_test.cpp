#include "lf/queue.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <random>
#include <stdexcept>
#include <thread>
#include <vector>

#include "gtest/gtest.h"

namespace lf {
namespace {

constexpr int CAPACITY = 10;

TEST(Queue, Size) {
  Queue<int> q{CAPACITY};
  EXPECT_EQ(q.size(), 0);
  q.push(42);
  EXPECT_EQ(q.size(), 1);
}

TEST(Queue, Empty) {
  Queue<int> q{CAPACITY};
  EXPECT_TRUE(q.empty());
  q.push(42);
  EXPECT_FALSE(q.empty());
}

TEST(Queue, MaxSize) {
  Queue<int> q{CAPACITY};
  EXPECT_EQ(q.max_size(), CAPACITY);
}

TEST(Queue, Capacity) {
  Queue<int> q{CAPACITY};
  EXPECT_EQ(q.capacity(), CAPACITY);
}

TEST(Queue, PushToCapacity) {
  Queue<int> q{CAPACITY};
  for (int i = 0; i < CAPACITY; ++i) {
    ASSERT_NO_THROW(q.push(i))
      << "Iteration " << i << "; Size " << q.size() << "; Capacity "
      << q.capacity();
  }

  EXPECT_EQ(q.size(), q.capacity());
  EXPECT_THROW(q.push(11), std::length_error);
}

TEST(Queue, PopInOrder) {
  Queue<int> q{CAPACITY};
  for (int i = 0; i < CAPACITY; ++i) {
    q.push(i);
  }

  for (int i = 0; i < CAPACITY; ++i) {
    ASSERT_FALSE(q.empty());
    EXPECT_EQ(q.pop(), i);
  }
  EXPECT_TRUE(q.empty());
}

TEST(Queue, PopWhileEmpty) {
  Queue<int> q{CAPACITY};
  ASSERT_TRUE(q.empty());
  EXPECT_EQ(q.pop(), std::nullopt);
}

TEST(Queue, InterleavePushPop) {
  Queue<int> q{CAPACITY};
  for (int i = 0; i < CAPACITY * 100; ++i) {
    q.push(i);
    EXPECT_EQ(q.size(), 1);
    EXPECT_EQ(q.pop(), i);
    EXPECT_TRUE(q.empty());
  }
}

TEST(Queue, MultithreadedReadWrite) {
  Queue<int> q{CAPACITY};
  const int limit = CAPACITY * 100;
  std::vector<int> output;
  output.reserve(limit);

  std::thread reader{[&]() {
    while (output.size() < limit) {
      std::optional<int> val = q.pop();
      if (val) output.push_back(*val);
    }
  }};
  std::thread writer{[&]() {
    for (int i = 0; i < limit; ++i) {
      while (q.size() == q.capacity()) continue;
      q.push(i);
    }
  }};

  reader.join();
  writer.join();

  EXPECT_TRUE(q.empty());
  EXPECT_EQ(output.size(), limit);
  for (int i = 0; i < limit; ++i) {
    EXPECT_EQ(output.at(i), i);
  }
}

TEST(Queue, UnbalancedMultithreadedReadWrite) {
  Queue<int> q{CAPACITY};
  const int limit = CAPACITY * 100;
  std::vector<int> output;
  output.reserve(limit);

  std::thread reader{[&]() {
    std::random_device rd;
    std::uniform_int_distribution<int> dist{1, 100};

    while (output.size() < limit) {
      std::optional<int> val = q.pop();
      if (val) output.push_back(*val);
      std::this_thread::sleep_for(std::chrono::microseconds{dist(rd)});
    }
  }};
  std::thread writer{[&]() {
    std::random_device rd;
    std::uniform_int_distribution<int> dist{1, 100};

    for (int i = 0; i < limit; ++i) {
      while (q.size() == q.capacity()) continue;
      q.push(i);
      std::this_thread::sleep_for(std::chrono::microseconds{dist(rd)});
    }
  }};

  reader.join();
  writer.join();

  EXPECT_TRUE(q.empty());
  EXPECT_EQ(output.size(), limit);
  for (int i = 0; i < limit; ++i) {
    EXPECT_EQ(output.at(i), i);
  }
}

TEST(Queue, MultithreadedSlowWriter) {
  Queue<int> q{CAPACITY};
  const int limit = CAPACITY * 10;
  std::vector<int> output;
  output.reserve(limit);

  std::thread reader{[&]() {
    while (output.size() < limit) {
      std::optional<int> val = q.pop();
      if (val) output.push_back(*val);
    }
  }};
  std::thread writer{[&]() {
    for (int i = 0; i < limit; ++i) {
      while (q.size() == q.capacity()) continue;
      q.push(i);
      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
  }};

  reader.join();
  writer.join();

  EXPECT_TRUE(q.empty());
  EXPECT_EQ(output.size(), limit);
  for (int i = 0; i < limit; ++i) {
    EXPECT_EQ(output.at(i), i);
  }
}

TEST(Queue, MultithreadedSlowReader) {
  Queue<int> q{CAPACITY};
  const int limit = CAPACITY * 10;
  std::vector<int> output;
  output.reserve(limit);

  std::thread reader{[&]() {
    while (output.size() < limit) {
      std::optional<int> val = q.pop();
      if (val) output.push_back(*val);
      std::this_thread::sleep_for(std::chrono::milliseconds{1});
    }
  }};
  std::thread writer{[&]() {
    for (int i = 0; i < limit; ++i) {
      while (q.size() == q.capacity()) continue;
      q.push(i);
    }
  }};

  reader.join();
  writer.join();

  EXPECT_TRUE(q.empty());
  EXPECT_EQ(output.size(), limit);
  for (int i = 0; i < limit; ++i) {
    EXPECT_EQ(output.at(i), i);
  }
}

}
}
