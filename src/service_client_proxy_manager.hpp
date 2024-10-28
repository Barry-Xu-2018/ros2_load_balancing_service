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

#include <chrono>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

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
  using SharedPtr = std::shared_ptr<ServiceClientProxyManager>;

  using ClientProxy = rclcpp::GenericClient;
  using SharedClientProxy = rclcpp::GenericClient::SharedPtr;

  using ClientProxyChangeCallbackType = std::function<bool (SharedClientProxy &)>;

  ServiceClientProxyManager(
    const std::string & base_service_name,
    const std::string & service_type,
    rclcpp::Node::SharedPtr & node,
    ResponseReceiveQueue::SharedPtr & response_queue,
    std::chrono::seconds discovery_interval = std::chrono::seconds(1));

  ~ServiceClientProxyManager();

  /**
   * @brief Set the callback function to be called when a load balancing service is added or
   *   removed.
   *
   * @param func_add A callback for adding a new load balancing service server
   * @param func_remove A callback for removing a load balancing service server
   */
  void set_client_proxy_change_callback(
    ClientProxyChangeCallbackType func_add,
    ClientProxyChangeCallbackType func_remove);

  /**
   * @brief Send a request to the load balancing service server using the specified service client
   *   proxy
   *
   * @param client_proxy Specified service client proxy
   * @param request Untyped request message
   * @param sequence Output sequence number of sending request
   * @return True if sending successfully, otherwise False.
   */
  bool
  async_send_request(
    SharedClientProxy & client_proxy,
    rclcpp::GenericService::SharedRequest & request,
    int64_t & sequence);

  // The following functions are provided for the discovery thread to call

  /**
   * @brief Create generic client to specified service name
   *
   * @param service_name service name to be connected by created generic client
   * @return A shared pointer to GenericClient.
   */
  SharedClientProxy
  create_service_proxy(const std::string service_name);

  /**
   * @brief Start a thread to periodically discover new load balancing service servers.
   */
  void
  start_discovery_service_servers_thread();

  /**
   * @brief Set a flag to terminate discovery service servers thread
   */
  void
  stop_discovery_thread_running(void);

  /**
   * @brief Check if discovery service servers thread is launched
   */
  bool
  is_discovery_thread_running(void);

  /**
   * @brief Remove a record of service server name and its corresponding service client proxy.
   * These records are maintained by the ServiceClientProxyManager itself.
   *
   * @param new_services load balancing service name to be removed.
   */
  void
  remove_load_balancing_service(const std::string & new_services);

  /**
   * @brief Activate a discovery operation once in the discovery thread.
   */
  void
  send_request_to_check_service_servers();

  /**
   * @brief Return the corresponding service client proxy based on the load balancing service name.
   *
   * @param service_name load balancing service name
   * @return service client proxy if found, otherwise nullptr
   */
  SharedClientProxy
  get_created_client_proxy(const std::string & service_name);

private:
  const std::string class_name_ = "ServiceClientProxyManager";
  rclcpp::Logger logger_;

  // */load_balancing/base_service_name_/1
  std::string base_service_name_;
  std::string service_type_;
  rclcpp::Node::SharedPtr node_;
  ResponseReceiveQueue::SharedPtr response_queue_;
  rclcpp::TimerBase::SharedPtr timer_;

  std::mutex callback_mutex_;
  ClientProxyChangeCallbackType add_callback_;
  ClientProxyChangeCallbackType remove_callback_;

  std::chrono::seconds discovery_interval_;
  std::thread discovery_service_server_thread_;
  std::atomic_bool thread_exit_{false};
  std::mutex cond_mutex_;
  std::condition_variable cv_;

  std::mutex registered_service_servers_info_mutex_;
  // Used service server list (corresponding service client proxy)
  std::unordered_map<std::string, SharedClientProxy> registered_service_servers_info_;

  // The index represents a request sent by a proxy client.
  std::atomic<u_int64_t> proxy_send_request_index{0};

  // Save future after sending request
  // SharedFuture <--> SharedClientProxy and proxy request sequence
  std::mutex client_proxy_futures_with_info_mutex_;
  std::unordered_map<u_int64_t,
    std::pair<SharedClientProxy, int64_t>> client_proxy_futures_with_info_;

  /**
   * @brief Check for changes in the load balancing service server.
   *
   * @return return two vectors. One stores the names of all added service servers and the other
   *   stores the names of all disappeared service servers.
   */
  std::pair<std::vector<std::string>, std::vector<std::string>>
  check_service_server_change();

  void
  discovery_service_server_thread(ServiceClientProxyManager::SharedPtr cli_proxy_mgr);

  // Discovery thread will use below 2 functions to notify load balancing service server change to
  // ForwardManagement.
  bool
  register_new_client_proxy(SharedClientProxy & cli_proxy);
  bool
  unregister_client_proxy(SharedClientProxy & cli_proxy);

  /**
   * @brief Record the name of a newly added service server and its corresponding service client
   *   proxy.
   * These records are maintained by the ServiceClientProxyManager itself.
   *
   * @param new_services A new load balancing service name
   * @param client_proxy Corresponding service client proxy
   */
  void
  add_new_load_balancing_service(
    const std::string & new_services,
    SharedClientProxy & client_proxy);

  void
  service_client_callback(rclcpp::GenericClient::SharedFuture future, u_int64_t send_index);

  uint64_t
  get_send_index();

  void
  wait_for_request_to_check_service_servers();
};

#endif  // SERVICE_CLIENT_PROXY_MANAGER_HPP_
