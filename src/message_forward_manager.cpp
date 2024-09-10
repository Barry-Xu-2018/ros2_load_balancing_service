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

#include <rclcpp/logging.hpp>

#include "message_forward_manager.hpp"

MessageForwardManager::MessageForwardManager(
  ServiceServerProxy::SharedPtr & srv_proxy,
  ServiceClientProxyManager::SharedPtr & cli_proxy_mgr,
  LoadBalancingProcess::SharedPtr & load_balancing_process,
  RequestReceiveQueue::SharedPtr & request_queue,
  ResponseReceiveQueue::SharedPtr & response_queue)
  : srv_proxy_(srv_proxy),
    cli_proxy_mgr_(cli_proxy_mgr),
    load_balancing_process_(load_balancing_process),
    request_queue_(request_queue),
    response_queue_(response_queue)
{
  // When a new service server is added, the corresponding client proxy will be created. The
  // LoadBalancingProcess will be notified through the MessageForwardManager.
  auto register_client_proxy =
    [this] (ServiceClientProxyManager::SharedClientProxy & cli_proxy) -> bool
    {
      return this->load_balancing_process_->register_client_proxy(cli_proxy);
    };

  // When a new service server is removed, the corresponding client proxy will be removed. The
  // LoadBalancingProcess will be notified through the MessageForwardManager.
  auto unregister_client_proxy =
    [this] (ServiceClientProxyManager::SharedClientProxy & cli_proxy) -> bool
    {
       return this->load_balancing_process_->unregister_client_proxy(cli_proxy);
    };

  cli_proxy_mgr_->set_client_proxy_change_callback(
    register_client_proxy,
    unregister_client_proxy);

}

MessageForwardManager::~MessageForwardManager()
{
  // Remove callback
  cli_proxy_mgr_->set_client_proxy_change_callback(
    nullptr,
    nullptr);
}

void MessageForwardManager::handle_request_process(
  RequestReceiveQueue::SharedPtr & request_queue,
  LoadBalancingProcess::SharedPtr & load_balancing_process,
  ServiceClientProxyManager::SharedPtr & cli_proxy_mgr)
{
  while (handle_request_thread_exit_.load())
  {
    request_queue->wait();

    auto ret = request_queue->out_queue();
    if (ret == std::nullopt) {
      RCLCPP_INFO(rclcpp::get_logger(class_name_),
        "Request queue is shutdown, exit request process thread.");
      break;
    }

    auto request = ret.value();
  
    auto client_proxy = load_balancing_process->request_client_proxy();
  }
}