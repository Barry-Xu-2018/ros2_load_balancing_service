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

#ifndef MESSAGE_FORWARD_PROCESS_HPP_
#define MESSAGE_FORWARD_PROCESS_HPP_

#include "data_queues.hpp"
#include "forward_management.hpp"
#include "service_client_proxy_manager.hpp"
#include "service_server_proxy.hpp"

/**
 * @brief Manage two threads: one is responsible for handling the request queue, and the other is
 *   responsible for handling the response queue
 */
class MessageForwardProcess {
public:
  MessageForwardProcess(
    ServiceServerProxy::SharedPtr & srv_proxy,
    ServiceClientProxyManager::SharedPtr & cli_proxy_mgr,
    ForwardManagement::SharedPtr & forward_management,
    RequestReceiveQueue::SharedPtr & request_queue,
    ResponseReceiveQueue::SharedPtr & response_queue);

  ~MessageForwardProcess();

private:
  const std::string class_name_ = "MessageForwardProcess";
  rclcpp::Logger logger_;

  ServiceServerProxy::SharedPtr srv_proxy_;
  ServiceClientProxyManager::SharedPtr cli_proxy_mgr_;
  ForwardManagement::SharedPtr forward_management_;
  RequestReceiveQueue::SharedPtr request_queue_;
  ResponseReceiveQueue::SharedPtr response_queue_;

  std::atomic_bool handle_response_thread_exit_{false};
  std::thread handle_response_thread_;
  std::atomic_bool handle_request_thread_exit_{false};
  std::thread handle_request_thread_;

  void handle_request_process(
    RequestReceiveQueue::SharedPtr & request_queue,
    ForwardManagement::SharedPtr & forward_management,
    ServiceClientProxyManager::SharedPtr & cli_proxy_mgr,
    rclcpp::Logger & logger);

  void handle_response_process(
    ResponseReceiveQueue::SharedPtr & response_queue,
    ForwardManagement::SharedPtr & forward_management,
    ServiceServerProxy::SharedPtr & srv_proxy,
    rclcpp::Logger & logger);
};

#endif  // MESSAGE_FORWARD_PROCESS_HPP_
