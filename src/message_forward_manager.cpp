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

  auto thread_handle_request_process = [this] () {
      handle_request_process(request_queue_, load_balancing_process_, cli_proxy_mgr_);
    };
  handle_request_thread_ = std::thread(thread_handle_request_process);

  auto thread_handle_response_process = [this] () {
    handle_response_process(response_queue_, load_balancing_process_, srv_proxy_);
  };
  handle_response_thread_ = std::thread(thread_handle_response_process);
}

MessageForwardManager::~MessageForwardManager()
{
  // Remove callback
  cli_proxy_mgr_->set_client_proxy_change_callback(
    nullptr,
    nullptr);

  // Stop thread for handling request
  request_queue_->shutdown();
  handle_request_thread_exit_.store(true);
  while (handle_request_thread_.joinable()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  // Stop thread for handling response
  response_queue_->shutdown();
  handle_response_thread_exit_.store(true);
  while (handle_response_thread_.joinable()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void MessageForwardManager::handle_request_process(
  RequestReceiveQueue::SharedPtr & request_queue,
  LoadBalancingProcess::SharedPtr & load_balancing_process,
  ServiceClientProxyManager::SharedPtr & client_proxy_mgr)
{
  while (handle_request_thread_exit_.load())
  {
    request_queue->wait();

    auto ret_value = request_queue->out_queue();
    if (ret_value == std::nullopt) {
      RCLCPP_INFO(rclcpp::get_logger(class_name_),
        "Request queue is shutdown, exit request process thread.");
      break;
    }

    auto request = ret_value.value();

    auto client_proxy = load_balancing_process->request_client_proxy();
    int64_t sequence_num;
    auto ret =
      client_proxy_mgr->async_send_request(
        client_proxy, std::get<SharedRequestMsg>(request) ,sequence_num);
    if (!ret) {
      RCLCPP_ERROR(rclcpp::get_logger(class_name_),
        "Failed to send request to %s", client_proxy->get_service_name());
      continue;
    }

    ret = load_balancing_process->add_one_record_to_corresponding_table(
      client_proxy, sequence_num, std::get<SharedRequestID>(request));
    if (!ret) {
      RCLCPP_ERROR(rclcpp::get_logger(class_name_),
        "Failed to record proxy client (%s, sequence:%ld)",
        client_proxy->get_service_name(), sequence_num);
      continue;
    }
  }
}

void MessageForwardManager::handle_response_process(
  ResponseReceiveQueue::SharedPtr & response_queue,
  LoadBalancingProcess::SharedPtr & load_balancing_process,
  ServiceServerProxy::SharedPtr & srv_proxy)
{
  while (handle_response_thread_exit_.load())
  {
    response_queue->wait();

    auto ret_value = response_queue->out_queue();
    if (ret_value == std::nullopt) {
      RCLCPP_INFO(rclcpp::get_logger(class_name_),
        "Response queue is shutdown, exit response process thread.");
      break;
    }

    auto response = ret_value.value();

    auto request_proxy_sequence = std::get<int64_t>(response);
    auto client_proxy = std::get<rclcpp::GenericClient::SharedPtr>(response);

    auto ret_request_id =
      load_balancing_process->get_request_info_from_corresponding_table(
        client_proxy, request_proxy_sequence);
    if (ret_request_id == std::nullopt) {
      RCLCPP_ERROR(rclcpp::get_logger(class_name_),
        "Failed to get request id based on client proxy (%s) and sequence (%ld).",
        client_proxy->get_service_name(), request_proxy_sequence);
      continue;
    }

    auto request_id = ret_request_id.value();
    srv_proxy->send_response(request_id, std::get<SharedResponseMsg>(response));
  }
}