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

#ifndef DATA_QUEUE_HPP_
#define DATA_QUEUE_HPP_

#include <array>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <tuple>

#include <rclcpp/rclcpp.hpp>

#include <rmw/types.h>

using WRITER_GUID = std::array<int8_t, RMW_GID_STORAGE_SIZE>;

template<typename T1, typename T2, typename T3>
class QueueBase {
public:
  using Data_Type = std::tuple<T1, T2, T3>;
  using SharedPtr = std::shared_ptr<QueueBase<T1, T2, T3>>;

  QueueBase() = default;
  ~QueueBase() = default;

  void in_queue(T1 & v1, T2 & v2, T3 & v3)
  {
    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      queue_.emplace(Data_Type(v1, v2, v3));
    }
    cv_.notify_one();
  }

  Data_Type out_queue(void)
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    auto data = queue_.front();
    queue_.pop();
    return data;
  }

  void wait(void) {
    std::unique_lock lock(cond_mutex_);
    cv_.wait(
      lock,
      [this] {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return !queue_.empty();}); 
  }

private:
  std::mutex queue_mutex_;
  std::queue<Data_Type> queue_;

  std::mutex cond_mutex_;
  std::condition_variable cv_;
};

// The queue for Service Server proxy to save received request from real service client
// Writer GUID, sequence, request
using RequestReceiveQueue = class QueueBase<WRITER_GUID, int64_t, std::shared_ptr<void>>;

// The queue for Service client proxy to save received response from real service server
// service name, sequence, response
using ResponseReceiveQueue =
  class QueueBase<const std::string, int64_t, std::shared_ptr<void>>;

#endif  // DATA_QUEUE_HPP_
