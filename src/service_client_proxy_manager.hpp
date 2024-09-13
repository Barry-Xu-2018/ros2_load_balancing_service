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
    std::chrono::seconds discovery_interval = std::chrono::seconds(2));

  ~ServiceClientProxyManager();

  SharedClientProxy create_service_proxy(const std::string service_name);

  void
  start_discovery_service_servers_thread();
  bool
  check_thread_status(void);

  std::pair<std::vector<std::string>, std::vector<std::string>>
  check_service_server_change();

  void
  add_new_load_balancing_service(
    const std::string & new_services,
    SharedClientProxy & client_proxy);
  void
  remove_new_load_balancing_service(const std::string & new_services);

  // LoadBalancingProccess will set add/remove callback
  void set_client_proxy_change_callback(
    ClientProxyChangeCallbackType func_add,
    ClientProxyChangeCallbackType func_remove);

  // Discovery thread will use below 2 functions to notify service server change to
  // LoadBalancingProccess
  bool
  register_new_client_proxy(SharedClientProxy & cli_proxy);
  bool
  unregister_client_proxy(SharedClientProxy & cli_proxy);

  // Control when the discovery operation needs to be executed in discovery thread
  void
  wait_for_request_to_check_service_servers();
  void
  send_request_to_check_service_servers();

  SharedClientProxy
  get_created_client_proxy(const std::string & service_name);

  bool
  async_send_request(
    SharedClientProxy & client_proxy,
    rclcpp::GenericService::SharedRequest & request,
    int64_t & sequence);

private:
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

  struct SharedFutureHash {
    std::size_t operator()(const rclcpp::GenericClient::SharedFuture & sf) const {
      return std::hash<void*>()(sf.get().get());
    }
  };

  struct SharedFutureEqual {
    bool operator()(const rclcpp::GenericClient::SharedFuture & lhs,
                    const rclcpp::GenericClient::SharedFuture & rhs) const {
        return lhs.get() == rhs.get();
    }
  };

  // Save future after sending request
  // SharedFuture <--> SharedClientProxy and proxy request sequence
  std::mutex client_proxy_futures_with_info_mutex_;
  std::unordered_map<rclcpp::GenericClient::SharedFuture,
    std::pair<SharedClientProxy, int64_t>,
    SharedFutureHash, SharedFutureEqual> client_proxy_futures_with_info_;

  void
  discovery_service_server_thread(ServiceClientProxyManager::SharedPtr cli_proxy_mgr);

  void
  service_client_callback(rclcpp::GenericClient::SharedFuture future);
};

#endif  // SERVICE_CLIENT_PROXY_MANAGER_HPP_
