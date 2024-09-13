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

#ifndef MESSAGE_FORWARD_MANAGER_HPP_
#define MESSAGE_FORWARD_MANAGER_HPP_

#include "data_queues.hpp"
#include "load_balancing_process.hpp"
#include "service_client_proxy_manager.hpp"
#include "service_server_proxy.hpp"

class MessageForwardManager {
public:
  MessageForwardManager(
    ServiceServerProxy::SharedPtr & srv_proxy,
    ServiceClientProxyManager::SharedPtr & cli_proxy_mgr,
    LoadBalancingProcess::SharedPtr & load_balancing_process,
    RequestReceiveQueue::SharedPtr & request_queue,
    ResponseReceiveQueue::SharedPtr & response_queue);

  ~MessageForwardManager();

private:
  const std::string class_name_ = "MessageForwardManager";
  rclcpp::Logger logger_;

  ServiceServerProxy::SharedPtr srv_proxy_;
  ServiceClientProxyManager::SharedPtr cli_proxy_mgr_;
  LoadBalancingProcess::SharedPtr load_balancing_process_;
  RequestReceiveQueue::SharedPtr request_queue_;
  ResponseReceiveQueue::SharedPtr response_queue_;

  std::atomic_bool handle_response_thread_exit_{false};
  std::thread handle_response_thread_;
  std::atomic_bool handle_request_thread_exit_{false};
  std::thread handle_request_thread_;

  void handle_request_process(
    RequestReceiveQueue::SharedPtr & request_queue,
    LoadBalancingProcess::SharedPtr & load_balancing_process,
    ServiceClientProxyManager::SharedPtr & cli_proxy_mgr,
    rclcpp::Logger & logger);

  void handle_response_process(
    ResponseReceiveQueue::SharedPtr & response_queue,
    LoadBalancingProcess::SharedPtr & load_balancing_process,
    ServiceServerProxy::SharedPtr & srv_proxy,
    rclcpp::Logger & logger);
};

#endif  // MESSAGE_FORWARD_MANAGER_HPP_
