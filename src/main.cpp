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

#include <chrono>
#include <cstdlib>
#include <string>
#include <memory>

#include "rclcpp/executors/multi_threaded_executor.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/logging.hpp"

#include "data_queues.hpp"
#include "forward_management.hpp"
#include "message_forward_process.hpp"
#include "service_client_proxy_manager.hpp"
#include "service_server_proxy.hpp"

void usage(char * name)
{
  printf("Usage:\n    %s [-h|--help] [-s|--service-name SERVICE_NAME] [-t|--service-type "
    "SERVICE_TYPE] [--strategy XXX] [-i|--interval TIME]\n"
    "       --strategy choose one of \"round_robin\", \"less_requests\" and \"less_response_time\"\n"
    "                  If not set, \"round_robin\" is used as default.\n"
    "                  \"round_robin\": select the service server in order.\n"
    "                  \"less_requests\": select the service server with the fewest requests.\n"
    "                  \"less_response_time\": select the service server with the shortest average response time.\n"
    "       --interval Interval to discovery service servers. Unit is second.\n"
    "                  If not set, default is 1s.\n",
    name);
}

#define FAIL_RETURN \
do {\
  rclcpp::shutdown(); \
  return EXIT_FAILURE; \
} while (0)

int main(int argc, char * argv[])
{
  const std::string node_name = "load_balancing_service_management";

  rclcpp::init(argc, argv);

  auto logger = rclcpp::get_logger("main");

  std::string service_name;
  std::string service_type;
  std::string load_balancing_strategy;
  std::string interval_str;

  // Parser parameters
  for (int i = 0; i < argc; ++i)
  {
    if ((std::strcmp(argv[i], "--service-name") == 0 ||
      std::strcmp(argv[i], "-s") == 0) && i + 1 < argc)
    {
      service_name = argv[i + 1];
      i++;
    }

    if ((std::strcmp(argv[i], "--service-type") == 0 ||
      std::strcmp(argv[i], "-t") == 0) && i + 1 < argc)
    {
      service_type = argv[i + 1];
      i++;
    }

    if (std::strcmp(argv[i], "--strategy") == 0 && i + 1 < argc) {
      load_balancing_strategy = argv[i + 1];
      i++;
    }

    if ((std::strcmp(argv[i], "--interval") == 0 ||
      std::strcmp(argv[i], "-i") == 0) && i + 1 < argc)
    {
      interval_str = argv[i + 1];
      i++;
    }

    if (std::strcmp(argv[i], "-h") == 0 || std::strcmp(argv[i], "--help") == 0) {
      usage(argv[0]);
      rclcpp::shutdown();
      return EXIT_SUCCESS;
    }
  }

  if (service_name.empty()) {
    RCLCPP_ERROR(logger,
      "Not set service name. Please specify -s SERVICE_NAME");
    FAIL_RETURN;
  } else {
    if (service_name[0] == '-') {
      RCLCPP_ERROR(logger,
        "Invalid service name \"%s\".", service_name.c_str());
      FAIL_RETURN;
    }
  }

  if (service_type.empty()) {
    RCLCPP_ERROR(logger,
      "Not set service type. Please specify -t SERVICE_TYPE");
    FAIL_RETURN;
  } else {
    if (service_type[0] == '-') {
      RCLCPP_ERROR(logger,
        "Invalid service type \"%s\".", service_type.c_str());
      FAIL_RETURN;
    }
  }

  if (load_balancing_strategy.empty()) {
    load_balancing_strategy = "round_robin";
  } else {
    if (!ForwardManagement::supported_load_balancing_strategy.count(load_balancing_strategy)) {
      RCLCPP_ERROR(logger,
        "Invalid strategy name \"%s\".\nPlease use one of \"round_robin\", "
        "\"less_requests\" and \"less_response_time\" as strategy name",
        load_balancing_strategy.c_str());
      FAIL_RETURN;
    }
  }

  uint32_t interval;
  if (interval_str.empty()) {
    interval = 1;
  } else {
    try {
      interval = static_cast<uint32_t>(std::stoul(interval_str));
    } catch (...) {
      RCLCPP_ERROR(logger,
        "Invalid input interval \"%s\".", interval_str.c_str());
      FAIL_RETURN;
    }
  }

  auto node = std::make_shared<rclcpp::Node>(
    node_name, rclcpp::NodeOptions().start_parameter_services(false));

  auto request_queue = std::make_shared<RequestReceiveQueue>();
  auto response_queue = std::make_shared<ResponseReceiveQueue>();

  auto forward_management = std::make_shared<ForwardManagement>(
    ForwardManagement::supported_load_balancing_strategy[load_balancing_strategy]);

  auto client_proxy_manager =
    std::make_shared<ServiceClientProxyManager>(
      service_name, service_type, node, response_queue, std::chrono::seconds(interval));

  // When a new service server is added or removed, the corresponding client proxy will be created
  // and removed. ServiceClientProxyManager will notify the change to ForwardManagement.
  client_proxy_manager->set_client_proxy_change_callback(
    [&forward_management](ServiceClientProxyManager::SharedClientProxy & cli_proxy) -> bool {
      return forward_management->register_client_proxy(cli_proxy);
    },
    [&forward_management](ServiceClientProxyManager::SharedClientProxy & cli_proxy) -> bool {
      return forward_management->unregister_client_proxy(cli_proxy);
    });

  auto service_proxy =
    std::make_shared<ServiceServerProxy>(service_name, service_type, node, request_queue);

  auto message_forward_process =
    std::make_shared<MessageForwardProcess>(
      service_proxy, client_proxy_manager, forward_management, request_queue, response_queue);

  // Start to discovery service server
  client_proxy_manager->start_discovery_service_servers_thread();

  RCLCPP_INFO(logger,
    "\n   Load balancing service name: %s\n"
    "                  Service type: %s\n"
    "       Load balancing strategy: %s\n"
    "  Interval of server discovery: %ds\n"
    "------------------------------\n"
    "Service client remap service name to %s\n"
    "Service server remap service name to %s/XXX\n"
    "------------------------------",
    service_proxy->get_service_name(), service_type.c_str(), load_balancing_strategy.c_str(),
    interval, service_proxy->get_service_name(), service_proxy->get_service_name());

  rclcpp::executors::MultiThreadedExecutor exec;
  exec.add_node(node);
  exec.spin();

  // Shutdown queue
  request_queue->shutdown();
  response_queue->shutdown();

  rclcpp::shutdown();

  return EXIT_SUCCESS;
}
