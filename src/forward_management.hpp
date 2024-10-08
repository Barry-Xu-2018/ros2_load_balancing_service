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

#ifndef FORWARD_MANAGEMENT_HPP_
#define FORWARD_MANAGEMENT_HPP_

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

/**
 * @brief Manage all service client proxies. Maintain the correspondence between the original
 * Request ID and the proxy request sequence.
 */
class ForwardManagement {
public:
  using SharedPtr = std::shared_ptr<ForwardManagement>;

  using SharedClientProxy = rclcpp::GenericClient::SharedPtr;
  using SharedServiceProxy = rclcpp::GenericService::SharedPtr;

  using SharedRequestID = std::shared_ptr<rmw_request_id_t>;
  using ProxyRequestSequence = int64_t;

  explicit ForwardManagement(
    LoadBalancingStrategy strategy = LoadBalancingStrategy::ROUND_ROBIN);

  ~ ForwardManagement(){}

  /**
   * @brief Register a service client proxy
   *
   * @param A shared generic client to be registered
   * @return False if Client proxy already existed, otherwise True.
   */
  bool
  register_client_proxy(SharedClientProxy & client);

  /**
   * @brief Unregister a service client proxy
   *
   * @param client A shared generic client to be unregistered
   * @return False if Client proxy is unregistered, otherwise True.
   */
  bool
  unregister_client_proxy(SharedClientProxy & client);

  /**
   * @brief Choose a service client proxy according to load balancing strategy
   *
   * @return A shared pointer to a service client proxy.
   */
  std::optional<SharedClientProxy>
  request_client_proxy();

  /**
   * @brief Add a record that describes the correspondence between the client proxy, proxy request
   * sequence, and the original request ID
   *
   * @param client_proxy A shared pointer to a service client proxy
   * @param proxy_request_sequence The sequence number of request managed by ServiceClientProxyManager
   * @param shared_request_id A shared pointer to rmw_request_id_t
   * @return False if the client proxy with proxy request sequence already existed, otherwise True.
   */
  bool
  add_one_record_to_corresponding_table(
    SharedClientProxy & client_proxy,
    ProxyRequestSequence proxy_request_sequence,
    SharedRequestID & shared_request_id);

  /**
   * @brief Get the original request ID according to the client proxy with proxy request sequence
   *
   * @param client_proxy A shared pointer to a service client proxy
   * @param proxy_request_sequence The sequence number of request managed by ServiceClientProxyManager
   * @return A shared pointer to rmw_request_id_t.
   */
  std::optional<SharedRequestID>
  get_request_info_from_corresponding_table(
    SharedClientProxy & client_proxy,
    ProxyRequestSequence proxy_request_sequence);

  static std::unordered_map<std::string, LoadBalancingStrategy> supported_load_balancing_strategy;

private:
  const std::string class_name_ = "ForwardManagement";
  rclcpp::Logger logger_;

  LoadBalancingStrategy strategy_;

  std::mutex client_proxy_info_mutex_;
  // LESS_REQUESTS only use first int64_t to save the number of sending request.
  // LESS_RESPONSE_TIME use second int64_t to save the average response time.
  std::unordered_map<SharedClientProxy, std::pair<int64_t, int64_t>> client_proxy_info_;
  // Pointing to the last iterator
  std::unordered_map<SharedClientProxy, std::pair<int64_t, int64_t>>::iterator round_robin_pointer_;

  std::mutex corresponding_table_mutex_;
  // After choosing proxy client and sending request message (get proxy request sequence), register
  // client proxy, proxy request sequence, request id
  std::unordered_map<SharedClientProxy, std::unordered_map<ProxyRequestSequence, SharedRequestID>>
    corresponding_table_;

  // Load balancing strategy implementation
  std::optional<SharedClientProxy>
  round_robin_to_choose_client_proxy();

  std::optional<SharedClientProxy>
  less_requests_to_choose_client_proxy();

  std::optional<SharedClientProxy>
  less_response_time_to_choose_client_proxy();

  std::mutex send_proxy_request_time_table_mutex_;
  // For less response time strategy
  using TimeType = std::chrono::time_point<std::chrono::steady_clock>;
  std::unordered_map<SharedClientProxy, std::unordered_map<ProxyRequestSequence, TimeType>>
    send_proxy_request_time_table_;

};

#endif  // FORWARD_MANAGEMENT_HPP_
