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

#include <cstdlib>
#include <string>
#include <memory>

#include "rclcpp/executors/multi_threaded_executor.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/logging.hpp"

#include "data_queues.hpp"
#include "load_balancing_process.hpp"
#include "message_forward_manager.hpp"
#include "service_client_proxy_manager.hpp"
#include "service_server_proxy.hpp"

int main(int argc, char * argv[])
{
  const std::string node_name = "load_balancing_service_management";

  rclcpp::init(argc, argv);

  auto node = std::make_shared<rclcpp::Node>(
    node_name, rclcpp::NodeOptions().start_parameter_services(false));

  auto srv_name = node->declare_parameter("srv_name", "");

  auto srv_type = node->declare_parameter("srv_type", "");

  auto load_balancing_strategy =
    node->declare_parameter("strategy", "");

  // Check input service name
  if (srv_name.empty()) {
    RCLCPP_ERROR(rclcpp::get_logger("main"),
      "Unknow service name. Please specify --ros-args -p srv_name:=");
    rclcpp::shutdown();
    return EXIT_FAILURE;
  }

  // Check input service type
  if (srv_type.empty()) {
    RCLCPP_ERROR(rclcpp::get_logger("main"),
      "Unknow service type. Please specify --ros-args -p srv_type:=");
    rclcpp::shutdown();
    return EXIT_FAILURE;
  }

  // Checking input loading balance strategy
  if (load_balancing_strategy.empty() ||
    !LoadBalancingProcess::supported_load_balancing_strategy.count(load_balancing_strategy)) {

    std::string all_supported_strategy;
    for (auto & pair: LoadBalancingProcess::supported_load_balancing_strategy) {
      all_supported_strategy += pair.first + ", ";
    }

    RCLCPP_ERROR(rclcpp::get_logger("main"),
      "Unknow strategy name. Please specify --ros-args -p strategy:=\n"
      "Please chose one of %s",
      (all_supported_strategy.erase(all_supported_strategy.size() - 2, 2)).c_str());
    rclcpp::shutdown();
    return EXIT_FAILURE;
  }

  auto request_queue = std::make_shared<RequestReceiveQueue>();
  auto response_queue = std::make_shared<ResponseReceiveQueue>();

  auto load_balancing_process = std::make_shared<LoadBalancingProcess>(
    LoadBalancingProcess::supported_load_balancing_strategy[load_balancing_strategy]);

  auto client_proxy_manager =
    std::make_shared<ServiceClientProxyManager>(srv_name, srv_type, node, response_queue);

  auto service_proxy =
    std::make_shared<ServiceServerProxy>(srv_name, srv_type, node, request_queue);

  auto message_forword_manager =
    std::make_shared<MessageForwardManager>(
      service_proxy, client_proxy_manager, load_balancing_process, request_queue, response_queue);

  // Start to discovery service server
  client_proxy_manager->start_discovery_service_servers_thread();

  RCLCPP_INFO(rclcpp::get_logger("main"),
    "\nLoad balancing service name: %s\n"
    "               Service type: %s\n"
    "    Load balancing strategy: %s",
    service_proxy->get_service_name(), srv_type.c_str(), load_balancing_strategy.c_str());

  rclcpp::executors::MultiThreadedExecutor exec;
  exec.add_node(node);
  exec.spin();

  // Disable process
  request_queue->shutdown();
  response_queue->shutdown();

  rclcpp::shutdown();

  return EXIT_SUCCESS;
}
