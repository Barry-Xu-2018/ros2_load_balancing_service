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

  ~MessageForwardManager(){
    // Remove callback
    cli_proxy_mgr_->set_client_proxy_change_callback(
      nullptr,
      nullptr);    
  }

private:
  ServiceServerProxy::SharedPtr srv_proxy_;
  ServiceClientProxyManager::SharedPtr cli_proxy_mgr_;
  LoadBalancingProcess::SharedPtr load_balancing_process_;
  RequestReceiveQueue::SharedPtr request_queue_;
  ResponseReceiveQueue::SharedPtr response_queue_;

  std::thread handle_response_thread_;
  std::thread handle_request_thread_;
};

#endif  // MESSAGE_FORWARD_MANAGER_HPP_