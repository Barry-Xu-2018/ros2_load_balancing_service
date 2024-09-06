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

#ifndef LOAD_BALANCING_POLICY_HPP_
#define LOAD_BALANCING_POLICY_HPP_

#include <unordered_map>

#include <rclcpp/generic_client.hpp>
#include <rclcpp/generic_service.hpp>
#include <rclcpp/logging.hpp>

/**
 * @brief All available load-balancing strategies
 */
enum class LoadBalancingStrategy { 
  ROUND_ROBIN, ///< Assign service servers sequentially according to the order of requests
  LESS_REQUESTS, ///< Select the service server with the fewest requests
  LESS_RESPONSE_TIME ///< Select the service server with the shortest average response time
};

class LoadBalancingProcess {
public:
  using SharedPtr = std::shared_ptr<LoadBalancingProcess>;

  using SharedClientProxy = rclcpp::GenericClient::SharedPtr;
  using SharedServiceProxy = rclcpp::GenericService::SharedPtr;

  explicit LoadBalancingProcess(LoadBalancingStrategy strategy = LoadBalancingStrategy::ROUND_ROBIN)
  : strategy_(strategy)
  {
  }

  bool register_client_proxy(SharedClientProxy & client) {
    auto found = client_proxy_info_.find(client);
    if (found != client_proxy_info_.end()) {
      RCLCPP_ERROR(rclcpp::get_logger(class_name),
        "Registering Client Proxy failed: Client Proxy already exist !");
      return false;
    }

    {
      std::lock_guard<std::mutex> lock(client_proxy_info_mutex_);
      client_proxy_info_[client] = 0;
    }
    return true;
  }

  bool unregister_client_proxy(SharedClientProxy & client) {
    auto found = client_proxy_info_.find(client);
    if (found == client_proxy_info_.end()) {
      RCLCPP_ERROR(rclcpp::get_logger(class_name),
        "Unregistering Client Proxy failed: Client Proxy doesn't exist !");
      return false;
    }

    {
      std::lock_guard<std::mutex> lock(client_proxy_info_mutex_);
      client_proxy_info_.erase(found);
    }
    return true;
  }

private:
  const std::string class_name = "LoadBalancingProcess";

  LoadBalancingStrategy strategy_;

  std::mutex client_proxy_info_mutex_;
  std::unordered_map<SharedClientProxy, int64_t> client_proxy_info_;
};

#endif  // LOAD_BALANCING_POLICY_HPP_