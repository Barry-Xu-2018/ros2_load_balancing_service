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

#include <thread>

#include <rclcpp/logging.hpp>

#include "message_forward_process.hpp"

MessageForwardProcess::MessageForwardProcess(
  ServiceServerProxy::SharedPtr & srv_proxy,
  ServiceClientProxyManager::SharedPtr & cli_proxy_mgr,
  ForwardManagement::SharedPtr & forward_management,
  RequestReceiveQueue::SharedPtr & request_queue,
  ResponseReceiveQueue::SharedPtr & response_queue)
  : logger_(rclcpp::get_logger(class_name_)),
    srv_proxy_(srv_proxy),
    cli_proxy_mgr_(cli_proxy_mgr),
    forward_management_(forward_management),
    request_queue_(request_queue),
    response_queue_(response_queue)
{
  #if 0
  // When a new service server is added, the corresponding client proxy will be created. The
  // LoadBalancingProcess will be notified through the MessageForwardManager.
  auto register_client_proxy =
    [this] (ServiceClientProxyManager::SharedClientProxy & cli_proxy) -> bool
    {
      return this->forward_management_->register_client_proxy(cli_proxy);
    };

  // When a new service server is removed, the corresponding client proxy will be removed. The
  // LoadBalancingProcess will be notified through the MessageForwardManager.
  auto unregister_client_proxy =
    [this] (ServiceClientProxyManager::SharedClientProxy & cli_proxy) -> bool
    {
       return this->forward_management_->unregister_client_proxy(cli_proxy);
    };

  cli_proxy_mgr_->set_client_proxy_change_callback(
    register_client_proxy,
    unregister_client_proxy);
  #endif

  auto thread_handle_request_process = [this] () {
      handle_request_process(request_queue_, forward_management_, cli_proxy_mgr_, logger_);
    };
  handle_request_thread_ = std::thread(thread_handle_request_process);

  auto thread_handle_response_process = [this] () {
    handle_response_process(response_queue_, forward_management_, srv_proxy_, logger_);
  };
  handle_response_thread_ = std::thread(thread_handle_response_process);
}

MessageForwardProcess::~MessageForwardProcess()
{
  // Remove callback
  cli_proxy_mgr_->set_client_proxy_change_callback(
    nullptr,
    nullptr);

  // Stop thread for handling request
  request_queue_->shutdown();
  handle_request_thread_exit_.store(true);
  handle_request_thread_.join();

  // Stop thread for handling response
  response_queue_->shutdown();
  handle_response_thread_exit_.store(true);
  handle_response_thread_.join();
}

void MessageForwardProcess::handle_request_process(
  RequestReceiveQueue::SharedPtr & request_queue,
  ForwardManagement::SharedPtr & forward_management,
  ServiceClientProxyManager::SharedPtr & client_proxy_mgr,
  rclcpp::Logger & logger)
{
  while (!handle_request_thread_exit_.load())
  {
    request_queue->wait();

    auto ret_value = request_queue->dequeue();
    if (!ret_value.has_value()) {
      // Request queue is shutdown, exit request handle thread.
      break;
    }

    auto request = ret_value.value();

    // Choose client proxy
    auto client_proxy = forward_management->request_client_proxy();
    while (client_proxy.has_value()) {
      if (client_proxy.value()->service_is_ready()) {
        break;
      }
      client_proxy_mgr->remove_load_balancing_service(client_proxy.value()->get_service_name());
      forward_management->unregister_client_proxy(client_proxy.value());
      client_proxy = forward_management->request_client_proxy();
    }
    if (!client_proxy.has_value()) {
      RCLCPP_ERROR(logger_, "No available client proxy to send request. "
       "The request is discarded.");
      continue;
    }

    int64_t sequence_num;
    auto ret =
      client_proxy_mgr->async_send_request(
        client_proxy.value(), std::get<SharedRequestMsg>(request) ,sequence_num);
    if (!ret) {
      RCLCPP_ERROR(logger_,
        "Failed to send request to %s", client_proxy.value()->get_service_name());
      continue;
    }

    RCLCPP_DEBUG(logger, "Forward request: [%02x %02x %02x %02x %02x %02x %02x %02x]:%ld => %p",
      std::get<SharedRequestID>(request)->writer_guid[0],
      std::get<SharedRequestID>(request)->writer_guid[1],
      std::get<SharedRequestID>(request)->writer_guid[2],
      std::get<SharedRequestID>(request)->writer_guid[3],
      std::get<SharedRequestID>(request)->writer_guid[4],
      std::get<SharedRequestID>(request)->writer_guid[5],
      std::get<SharedRequestID>(request)->writer_guid[6],
      std::get<SharedRequestID>(request)->writer_guid[7],
      std::get<SharedRequestID>(request)->sequence_number,
      static_cast<void *>(client_proxy.value().get()));

    ret = forward_management->add_one_record_to_corresponding_table(
      client_proxy.value(), sequence_num, std::get<SharedRequestID>(request));
    if (!ret) {
      RCLCPP_ERROR(logger_,
        "Failed to record proxy client (%s, sequence:%ld)",
        client_proxy.value()->get_service_name(), sequence_num);
      continue;
    }
  }
  RCLCPP_INFO(logger_, "Request handle thread exits.");
}

void MessageForwardProcess::handle_response_process(
  ResponseReceiveQueue::SharedPtr & response_queue,
  ForwardManagement::SharedPtr & forward_management,
  ServiceServerProxy::SharedPtr & srv_proxy,
  rclcpp::Logger & logger)
{
  while (!handle_response_thread_exit_.load())
  {
    response_queue->wait();

    auto ret_value = response_queue->dequeue();
    if (ret_value == std::nullopt) {
      // Response queue is shutdown, exit response handle thread.
      break;
    }

    auto response = ret_value.value();

    auto request_proxy_sequence = std::get<int64_t>(response);
    auto client_proxy = std::get<rclcpp::GenericClient::SharedPtr>(response);

    auto ret_request_id =
      forward_management->get_request_info_from_corresponding_table(
        client_proxy, request_proxy_sequence);
    if (ret_request_id == std::nullopt) {
      RCLCPP_ERROR(logger_,
        "Failed to get request id based on client proxy (%s) and sequence (%ld).",
        client_proxy->get_service_name(), request_proxy_sequence);
      continue;
    }

    auto request_id = ret_request_id.value();
    srv_proxy->send_response(request_id, std::get<SharedResponseMsg>(response));

    RCLCPP_DEBUG(logger, "Forward response: [%02x %02x %02x %02x %02x %02x %02x %02x]:%ld <= %p",
      request_id->writer_guid[0], request_id->writer_guid[1], request_id->writer_guid[2],
      request_id->writer_guid[3], request_id->writer_guid[4], request_id->writer_guid[5],
      request_id->writer_guid[6], request_id->writer_guid[7], request_id->sequence_number,
      static_cast<void *>(client_proxy.get()));
  }
  RCLCPP_INFO(logger_, "Response handle thread exists.");
}
