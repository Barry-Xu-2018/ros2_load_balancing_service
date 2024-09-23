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

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <tuple>

#include <rclcpp/rclcpp.hpp>
#include <rmw/types.h>

/**
 * @brief Provide basic queue management functionality.
 */
template<typename T1, typename T2, typename T3>
class QueueBase {
public:
  using Data_Type = std::tuple<T1, T2, T3>;
  using SharedPtr = std::shared_ptr<QueueBase<T1, T2, T3>>;

  QueueBase() = default;
  ~QueueBase() = default;

  void in_queue(T1 & v1, T2 & v2, T3 & v3)
  {
    if (shutdown_.load()) {
      return;
    }

    {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      queue_.emplace(Data_Type(v1, v2, v3));
    }
    cv_.notify_one();
  }

  std::optional<Data_Type> out_queue(void)
  {
    if (shutdown_.load()) {
      return std::nullopt;
    } else {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      auto data = queue_.front();
      queue_.pop();
      return data;
    }
  }

  void wait(void) {
    std::unique_lock lock(cond_mutex_);
    cv_.wait(
      lock,
      [this] {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        return !queue_.empty() || shutdown_.load();});
  }

  void shutdown(void) {
    shutdown_.store(true);
    cv_.notify_one();
  }

private:
  std::mutex queue_mutex_;
  std::queue<Data_Type> queue_;

  std::mutex cond_mutex_;
  std::condition_variable cv_;

  std::atomic_bool shutdown_{false};
};

using SharedRequestID = std::shared_ptr<rmw_request_id_t>;
using SharedRequestMsg = std::shared_ptr<void>;
using SharedResponseMsg = std::shared_ptr<void>;

// The queue for Service Server proxy to save received request from real service client
// std::shared_ptr<rmw_request_id_t>, Not_Used, request
using RequestReceiveQueue =
  class QueueBase<SharedRequestID, int64_t, SharedRequestMsg>;

// The queue for Service client proxy to save received response from real service server
// client proxy, sequence, response
using ResponseReceiveQueue =
  class QueueBase<rclcpp::GenericClient::SharedPtr, int64_t, SharedResponseMsg>;

#endif  // DATA_QUEUE_HPP_
