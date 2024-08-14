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

#ifndef REQUEST_RESPONSE_TABLE_HPP_
#define REQUEST_RESPONSE_TABLE_HPP_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <utility>
#include <functional>

#include <rmw/types.h>

using WRITER_GUID = int8_t[RMW_GID_STORAGE_SIZE];

class ServiceClientProxy;

using request_identify = std::pair<WRITER_GUID, int64_t>;
using response_identify = std::pair<std::shared_ptr<ServiceClientProxy>, int64_t>;

struct HastMethod {
  std::size_t operator()(const response_identify & p) const {
    auto hash1 = std::hash<std::shared_ptr<ServiceClientProxy>>{}(p.first);
    auto hash2 = std::hash<int64_t>{}(p.second);
    return hash1 ^ (hash2 << 1);
  }
};

class RequestResponseIdentifyTable {
public:
  // Insert data
  void insert(const response_identify & a, const request_identify & b)
  {
    table_.try_emplace(a, b);
  }

  // Get request identify via response identify
  request_identify get_request_identify(const response_identify & response_id) const {
      return table_.at(response_id);
  }

  // Check if response identify exists
  bool contains_response_identify(const response_identify & response_id) const {
      return table_.find(response_id) != table_.end();
  }

private:
  std::mutex map_mutex_;
  std::unordered_map<response_identify, request_identify, HastMethod> table_;
};

#endif  // REQUEST_RESPONSE_TABLE_HPP_
