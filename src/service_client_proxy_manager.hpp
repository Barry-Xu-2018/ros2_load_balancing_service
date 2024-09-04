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

#ifndef SERVICE_CLIENT_PROXY_MANAGER_HPP_
#define SERVICE_CLIENT_PROXY_MANAGER_HPP_

#include <memory>
#include <string>

#include "rclcpp/generic_client.hpp"
#include "rclcpp/node.hpp"

#include "data_queues.hpp"

/**
 * @brief Managing all service client proxies
 *
 * This class is responsible for managing all created service client proxies. Whenever a real
 * service server is detected to have started, the corresponding service client proxy will be
 * created to connect to this service server.
 */
class ServiceClientProxyManager : public std::enable_shared_from_this<ServiceClientProxyManager>
{
public:
  ServiceClientProxyManager(
    const std::string & base_service_name,
    const std::string & service_type,
    rclcpp::Node::SharedPtr node);

private:
  std::string base_service_name_;
  std::string service_type_;
  rclcpp::Node::SharedPtr node_;
  std::shared_ptr<Response_Receive_Queue> queue_;
};

#endif  // SERVICE_CLIENT_PROXY_MANAGER_HPP_
