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

#include <optional>
#include <unordered_map>

#include <rclcpp/generic_client.hpp>
#include <rclcpp/generic_service.hpp>
#include <rclcpp/logging.hpp>
#include <rmw/types.h>

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

  using SharedRequestID = std::shared_ptr<rmw_request_id_t>;
  using ProxyRequestSequence = int64_t;

  explicit LoadBalancingProcess(LoadBalancingStrategy strategy = LoadBalancingStrategy::ROUND_ROBIN)
  : strategy_(strategy)
  {
  }

  bool
  register_client_proxy(SharedClientProxy & client);

  bool unregister_client_proxy(SharedClientProxy & client);

  // The thread to handle request from service client will call this function to get service client
  // proxy to forward request to service server.
  SharedClientProxy
  request_client_proxy();

  bool
  add_one_record_to_corresponding_table(
    SharedClientProxy & client_proxy,
    ProxyRequestSequence proxy_request_sequence,
    SharedRequestID & shared_request_id);

  std::optional<SharedRequestID>
  get_request_info_from_corresponding_table(
    SharedClientProxy & client_proxy,
    ProxyRequestSequence proxy_request_sequence);

private:
  const std::string class_name_ = "LoadBalancingProcess";

  LoadBalancingStrategy strategy_;

  std::mutex client_proxy_info_mutex_;
  std::unordered_map<SharedClientProxy, int64_t> client_proxy_info_;
  // Pointing to the last iterator
  std::unordered_map<SharedClientProxy, int64_t>::iterator round_robin_pointer_;

  std::mutex corresponding_table_mutex_;
  // After choosing proxy client and sending request message (get proxy request sequence), register
  // client proxy, proxy request sequence , request id
  std::unordered_map<SharedClientProxy, std::unordered_map<ProxyRequestSequence, SharedRequestID>>
    corresponding_table_;

  // Load balancing strategy implementation
  SharedClientProxy round_robin_to_choose_client_proxy();
  SharedClientProxy less_requests_to_choose_client_proxy();
  SharedClientProxy less_response_time_to_choose_client_proxy();
};

#endif  // LOAD_BALANCING_POLICY_HPP_