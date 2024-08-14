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

#include <memory>
#include <mutex>
#include <queue>
#include <tuple>

#include <rmw/types.h>

#include "service_client_proxy.hpp"

using GUID = int8_t[RMW_GID_STORAGE_SIZE];

template<typename T1, typename T2, typename T3>
class QueueBase {
public:
  using Data_Type = std::tuple<T1, T2, T3>;

  QueueBase() = default;
  ~QueueBase() = default;

  void in_queue(T1 v1, T2 v2, T3 v3)
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    queue_.emplace(Data_Type(v1, v2, v3));
  }

  Data_Type out_queue(void)
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    auto data = queue_.front();
    queue_.pop();
    return data;
  }

private:
  std::mutex queue_mutex_;
  std::queue<Data_Type> queue_;
};

using Request_Receive_Queue = class QueueBase<GUID, int64_t, std::shared_ptr<void>>;
using Response_Receive_Queue = class QueueBase<std::shared_ptr<ServiceClientProxy>, int64_t, std::shared_ptr<void>>;

#endif  // DATA_QUEUE_HPP_
