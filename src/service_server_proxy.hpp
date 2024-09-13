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

#ifndef SERVICE_SERVER_PROXY_HPP_
#define SERVICE_SERVER_PROXY_HPP_

#include <memory>
#include <string>

#include <rclcpp/generic_service.hpp>
#include <rmw/types.h>

#include "data_queues.hpp"

class ServiceServerProxy : public std::enable_shared_from_this<ServiceServerProxy>
{
public:
  using SharedPtr = std::shared_ptr<ServiceServerProxy>;
  using SharedRequestID = std::shared_ptr<rmw_request_id_t>;

  ServiceServerProxy(
    const std::string & base_service_name,
    const std::string & service_type,
    rclcpp::Node::SharedPtr & node,
    RequestReceiveQueue::SharedPtr & request_queue);
  ~ServiceServerProxy();

  void send_response(
    const SharedRequestID & request_id,
    rclcpp::GenericService::SharedResponse response);

  const char * get_service_name();

private:
  const std::string class_name_ = "ServiceServerProxy";
  rclcpp::Logger logger_;

  const std::string base_service_name_;
  const std::string service_type_;
  rclcpp::Node::SharedPtr node_;
  RequestReceiveQueue::SharedPtr request_queue_;

  rclcpp::GenericService::SharedPtr service_server_proxy_;

  void callback_receive_request(
      SharedRequestID & request_id,
      rclcpp::GenericService::SharedRequest & request);
};

#endif  // SERVICE_SERVER_PROXY_HPP_
