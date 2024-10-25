// Copyright 2024 Sony Group Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gtest/gtest.h>

#include <thread>
#include <chrono>

#include "../src/data_queues.hpp"

using TestQueue = class QueueBase<int64_t, int64_t, int64_t>;


TEST(TestDataQueues, test_in_queue_and_wait)
{
  TestQueue test_queue;

  std::thread wait_thread([&test_queue](){
    test_queue.wait();
    EXPECT_EQ(test_queue.queue_size(), 1);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  int64_t val[3] = {1, 2, 3};
  test_queue.in_queue(val[0], val[1], val[2]);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  if (wait_thread.joinable()) {
    test_queue.shutdown();
    wait_thread.join();
  }
}

TEST(TestDataQueues, test_out_queue)
{
  TestQueue test_queue;
  int64_t val[3] = {1, 2, 3};

  std::thread wait_thread([&test_queue, &val](){
    test_queue.wait();
    ASSERT_EQ(test_queue.queue_size(), 1);
    auto out = test_queue.out_queue();
    ASSERT_TRUE(out.has_value());
    EXPECT_EQ(std::get<0>(out.value()), val[0]);
    EXPECT_EQ(std::get<1>(out.value()), val[1]);
    EXPECT_EQ(std::get<2>(out.value()), val[2]);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  test_queue.in_queue(val[0], val[1], val[2]);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  if (wait_thread.joinable()) {
    test_queue.shutdown();
    wait_thread.join();
  }
}

TEST(TestDataQueues, test_shutdown)
{
  TestQueue test_queue;

  std::thread wait_thread([&test_queue](){
    test_queue.wait();
    EXPECT_EQ(test_queue.queue_size(), 0);
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  test_queue.shutdown();

  if (wait_thread.joinable()) {
    wait_thread.join();
  }
}

TEST(TestDataQueues, test_out_order)
{
  TestQueue test_queue;
  int64_t val[3][3] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
  };

  test_queue.in_queue(val[0][0], val[0][1], val[0][2]);
  test_queue.in_queue(val[1][0], val[1][1], val[1][2]);
  test_queue.in_queue(val[2][0], val[2][1], val[2][2]);

  for (int i = 0; i < 3; i++) {
    auto out = test_queue.out_queue();
    EXPECT_TRUE(out.has_value());
    EXPECT_EQ(std::get<0>(out.value()), val[i][0]);
    EXPECT_EQ(std::get<1>(out.value()), val[i][1]);
    EXPECT_EQ(std::get<2>(out.value()), val[i][2]);
  }

  auto out = test_queue.out_queue();
  EXPECT_FALSE(out.has_value());
}

TEST(TestDataQueues, test_queue_size)
{
  TestQueue test_queue;

  EXPECT_EQ(test_queue.queue_size(), 0);

  int64_t val[3][3] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
  };

  test_queue.in_queue(val[0][0], val[0][1], val[0][2]);
  test_queue.in_queue(val[1][0], val[1][1], val[1][2]);
  test_queue.in_queue(val[2][0], val[2][1], val[2][2]);

  EXPECT_EQ(test_queue.queue_size(), 3);
}
