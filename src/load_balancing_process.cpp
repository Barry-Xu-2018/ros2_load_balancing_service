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

#include <cstdint>

#include <rclcpp/logging.hpp>

#include "load_balancing_process.hpp"

std::unordered_map<std::string, LoadBalancingStrategy>
  LoadBalancingProcess::supported_load_balancing_strategy = {
  {"round_robin", LoadBalancingStrategy::ROUND_ROBIN},
  {"less_requests", LoadBalancingStrategy::LESS_REQUESTS},
  {"less_response_time", LoadBalancingStrategy::LESS_RESPONSE_TIME}
};

bool
LoadBalancingProcess::register_client_proxy(SharedClientProxy & client) {
  std::lock_guard<std::mutex> lock(client_proxy_info_mutex_);

  auto found = client_proxy_info_.find(client);
  if (found != client_proxy_info_.end()) {
    RCLCPP_ERROR(logger_,
      "Registering Client Proxy failed: Client Proxy already exist !");
    return false;
  }

  // For initialization, the number of users is set to 0.
  client_proxy_info_[client] = 0;
  return true;
}

bool
LoadBalancingProcess::unregister_client_proxy(SharedClientProxy & client) {
  // clean client proxy list
  {
    std::lock_guard<std::mutex> lock(client_proxy_info_mutex_);

    auto found = client_proxy_info_.find(client);
    if (found == client_proxy_info_.end()) {
      RCLCPP_ERROR(logger_,
        "Unregistering Client Proxy failed: Client Proxy doesn't exist !");
      return false;
    }

    client_proxy_info_.erase(found);

    if (strategy_ == LoadBalancingStrategy::ROUND_ROBIN) {
      round_robin_pointer_ = client_proxy_info_.end();
    }
  }

  // Clean corresponding table
  {
    std::lock_guard<std::mutex> lock(corresponding_table_mutex_);
    if (corresponding_table_.count(client)) {
      corresponding_table_.erase(client);
    }
  }

  if (strategy_ == LoadBalancingStrategy::LESS_RESPONSE_TIME) {
    std::lock_guard<std::mutex> lock(send_proxy_request_time_table_mutex_);
    send_proxy_request_time_table_.erase(client);
  }

  return true;
}

LoadBalancingProcess::SharedClientProxy
LoadBalancingProcess::round_robin_to_choose_client_proxy()
{
  std::lock_guard<std::mutex> lock(client_proxy_info_mutex_);

  if (client_proxy_info_.empty()) {
    return nullptr;
  }

  if (round_robin_pointer_ == client_proxy_info_.end()
      || ++round_robin_pointer_ == client_proxy_info_.end()) {
    round_robin_pointer_ = client_proxy_info_.begin();
  }

  round_robin_pointer_->second++;
  return round_robin_pointer_->first;
}

LoadBalancingProcess::SharedClientProxy
LoadBalancingProcess::less_requests_to_choose_client_proxy()
{
  std::lock_guard<std::mutex> lock(client_proxy_info_mutex_);

  if (client_proxy_info_.empty()) {
    return nullptr;
  }

  LoadBalancingProcess::SharedClientProxy return_client_proxy;
  int64_t min_used_num = INT64_MAX;
  for (auto & [client_proxy, used_num]:client_proxy_info_) {
    if (used_num == 0) {
      return_client_proxy = client_proxy;
      break;
    }

    if (used_num < min_used_num) {
      return_client_proxy = client_proxy;
      min_used_num = used_num;
    }
  }

  return return_client_proxy;
}

LoadBalancingProcess::SharedClientProxy
LoadBalancingProcess::less_response_time_to_choose_client_proxy()
{
  // Calculation method is the same as the less request strategy
  return less_requests_to_choose_client_proxy();
}

LoadBalancingProcess::SharedClientProxy
LoadBalancingProcess::request_client_proxy()
{
  if (strategy_ == LoadBalancingStrategy::ROUND_ROBIN) {
    return round_robin_to_choose_client_proxy();
  }

  if (strategy_ == LoadBalancingStrategy::LESS_REQUESTS) {
    return less_requests_to_choose_client_proxy();
  }

  if (strategy_ == LoadBalancingStrategy::LESS_REQUESTS) {
    return less_response_time_to_choose_client_proxy();
  }

  return nullptr;
}

bool
LoadBalancingProcess::add_one_record_to_corresponding_table(
  SharedClientProxy & client_proxy,
  ProxyRequestSequence proxy_request_sequence,
  SharedRequestID & shared_request_id)
{
  {
    std::lock_guard<std::mutex> lock(corresponding_table_mutex_);

    // Check if client_proxy + proxy_request_sequence already existed in corresponding_table_
    if (corresponding_table_.count(client_proxy)
      && corresponding_table_[client_proxy].count(proxy_request_sequence))
    {
      RCLCPP_ERROR(logger_,
        "The sequence of request of service client proxy already existed.");
      return false;
    }

    corresponding_table_[client_proxy][proxy_request_sequence]
      = shared_request_id;
  }

  switch (strategy_) {
    case LoadBalancingStrategy::LESS_REQUESTS:
      {
        std::lock_guard<std::mutex> lock(client_proxy_info_mutex_);
        client_proxy_info_[client_proxy]++;
      }
      break;
    case LoadBalancingStrategy::LESS_RESPONSE_TIME:
      {
        std::lock_guard<std::mutex> lock(send_proxy_request_time_table_mutex_);
        send_proxy_request_time_table_[client_proxy][proxy_request_sequence]
          = std::chrono::steady_clock::now();
      }
      break;
    default:
      break;
  }

  return true;
}

std::optional<LoadBalancingProcess::SharedRequestID>
LoadBalancingProcess::get_request_info_from_corresponding_table(
  SharedClientProxy & client_proxy,
  ProxyRequestSequence proxy_request_sequence)
{
  std::optional<LoadBalancingProcess::SharedRequestID> ret_value = std::nullopt;
  {
    std::lock_guard<std::mutex> lock(corresponding_table_mutex_);
    if (!corresponding_table_.count(client_proxy)
      || !corresponding_table_[client_proxy].count(proxy_request_sequence))
    {
      RCLCPP_WARN(logger_,
        "No client proxy or proxy request sequence exist in corresponding table.");
      return ret_value;
    }
    ret_value = corresponding_table_[client_proxy][proxy_request_sequence];
    corresponding_table_[client_proxy].erase(proxy_request_sequence);
  }

  switch (strategy_) {
    case LoadBalancingStrategy::LESS_REQUESTS:
      // Reduce the number of processed requests
      {
        std::lock_guard<std::mutex> local(client_proxy_info_mutex_);
        client_proxy_info_[client_proxy]--;
      }
      break;
    case LoadBalancingStrategy::LESS_RESPONSE_TIME:
      // Update the average processing time of the response.
      {
        TimeType send_time;
        {
          std::lock_guard<std::mutex> lock(send_proxy_request_time_table_mutex_);
          send_time = send_proxy_request_time_table_[client_proxy][proxy_request_sequence];
          send_proxy_request_time_table_[client_proxy].erase(proxy_request_sequence);
        }
        auto response_used_time =
          std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - send_time);

        // Update the average response time of the client proxy
        {
          std::lock_guard<std::mutex> local_lock(client_proxy_info_mutex_);
          if (client_proxy_info_.count(client_proxy)) {
            client_proxy_info_[client_proxy] =
              (client_proxy_info_[client_proxy] + response_used_time.count())/2;
          }
        }
      }
      break;
    default:
      break;
  }

  return ret_value;
}
