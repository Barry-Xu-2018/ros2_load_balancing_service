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

#include <array>

#include <rclcpp/logging.hpp>

#include "common.hpp"
#include "service_server_proxy.hpp"

ServiceServerProxy::ServiceServerProxy(
  const std::string & base_service_name,
  const std::string & service_type,
  rclcpp::Node::SharedPtr & node,
  RequestReceiveQueue::SharedPtr & request_queue)
: logger_(rclcpp::get_logger(class_name_)),
  base_service_name_(base_service_name),
  service_type_(service_type),
  node_(node),
  request_queue_(request_queue)
{
  auto service_proxy_name = PREFIX_LOAD_BALANCING + "/" + base_service_name;

  auto callback = [this]
    (std::shared_ptr<rmw_request_id_t> request_id, rclcpp::GenericService::SharedRequest request)
    {
      callback_receive_request(request_id, request);
    };

  service_server_proxy_ = node->create_generic_service(
    service_proxy_name, service_type, callback);
}

ServiceServerProxy::~ServiceServerProxy()
{
}

void ServiceServerProxy::callback_receive_request(
  SharedRequestID & request_id,
  rclcpp::GenericService::SharedRequest & request)
{
#if 0
  RCLCPP_DEBUG(logger_,
    "Receive request from [%02x %02x %02x %02x %02x %02x %02x %02x]:%ld",
    request_id->writer_guid[0], request_id->writer_guid[1], request_id->writer_guid[2],
    request_id->writer_guid[3], request_id->writer_guid[4], request_id->writer_guid[5],
    request_id->writer_guid[6], request_id->writer_guid[7], request_id->sequence_number);
#endif
  request_queue_->enqueue(request_id, request_id->sequence_number, request);
}

void ServiceServerProxy::send_response(
  const SharedRequestID & request_id,
  rclcpp::GenericService::SharedResponse response)
{
  service_server_proxy_->send_response(*request_id, response);
}

const char * ServiceServerProxy::get_service_name()
{
  return service_server_proxy_->get_service_name();
}
