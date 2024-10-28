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

#include <gtest/gtest.h>

#include <rclcpp/rclcpp.hpp>

#include "std_srvs/srv/empty.hpp"

#include "../src/forward_management.hpp"

class TestForwardManagement : public ::testing::Test
{
protected:
  static void SetUpTestCase()
  {
    rclcpp::init(0, nullptr);
  }

  static void TearDownTestCase()
  {
    rclcpp::shutdown();
  }

  void SetUp()
  {
    node_ = std::make_shared<rclcpp::Node>("test_node",
      rclcpp::NodeOptions().start_parameter_services(false));
    client_proxy1_ = node_->create_generic_client(
      "/load_balancing/test_service/s1", "std_srvs/srv/Empty");
    client_proxy2_ = node_->create_generic_client(
      "/load_balancing/test_service/s2", "std_srvs/srv/Empty");
    client_proxy3_ = node_->create_generic_client(
      "/load_balancing/test_service/s3", "std_srvs/srv/Empty");
  }

  void TearDown()
  {
    node_.reset();
  }

  void register_3_client_proxy(ForwardManagement & forward_management)
  {
    ASSERT_TRUE(forward_management.register_client_proxy(client_proxy1_));
    ASSERT_TRUE(forward_management.register_client_proxy(client_proxy2_));
    ASSERT_TRUE(forward_management.register_client_proxy(client_proxy3_));
  }

  rclcpp::Node::SharedPtr node_;
  rclcpp::GenericClient::SharedPtr client_proxy1_;
  rclcpp::GenericClient::SharedPtr client_proxy2_;
  rclcpp::GenericClient::SharedPtr client_proxy3_;
};

TEST_F(TestForwardManagement, test_register_client_proxy)
{
  ForwardManagement forward_management(LoadBalancingStrategy::ROUND_ROBIN);

  EXPECT_TRUE(forward_management.register_client_proxy(client_proxy1_));
  EXPECT_TRUE(forward_management.register_client_proxy(client_proxy2_));
  EXPECT_TRUE(forward_management.register_client_proxy(client_proxy3_));

  // register repeated client proxy
  EXPECT_FALSE(forward_management.register_client_proxy(client_proxy2_));
}

TEST_F(TestForwardManagement, test_unregister_client_proxy)
{
  ForwardManagement forward_management(LoadBalancingStrategy::ROUND_ROBIN);

  register_3_client_proxy(forward_management);

  EXPECT_TRUE(forward_management.unregister_client_proxy(client_proxy2_));
  EXPECT_TRUE(forward_management.unregister_client_proxy(client_proxy1_));

  // unregister client proxy which is already removed.
  EXPECT_FALSE(forward_management.unregister_client_proxy(client_proxy2_));
}

TEST_F(TestForwardManagement, test_add_one_record_to_corresponding_table)
{
  // For testing, use the same Request ID.
  ForwardManagement::SharedRequestID requst_id = std::make_shared<rmw_request_id_t>();

  ForwardManagement forward_management(LoadBalancingStrategy::ROUND_ROBIN);

  register_3_client_proxy(forward_management);

  EXPECT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy1_, 1, requst_id));
  EXPECT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy2_, 1, requst_id));
  EXPECT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy3_, 1, requst_id));

  EXPECT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy2_, 2, requst_id));
  EXPECT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy3_, 2, requst_id));

  EXPECT_FALSE(
    forward_management.add_one_record_to_corresponding_table(client_proxy2_, 1, requst_id));

  EXPECT_FALSE(
    forward_management.add_one_record_to_corresponding_table(client_proxy3_, 2, requst_id));
}

TEST_F(TestForwardManagement, test_get_request_info_from_corresponding_table)
{
  ForwardManagement::SharedRequestID client_proxy_1_1_requst_id = std::make_shared<rmw_request_id_t>();
  ForwardManagement::SharedRequestID client_proxy_2_1_requst_id = std::make_shared<rmw_request_id_t>();
  ForwardManagement::SharedRequestID client_proxy_1_2_requst_id = std::make_shared<rmw_request_id_t>();
  ForwardManagement::SharedRequestID client_proxy_1_3_requst_id = std::make_shared<rmw_request_id_t>();
  ForwardManagement::SharedRequestID client_proxy_2_2_requst_id = std::make_shared<rmw_request_id_t>();

  ForwardManagement forward_management(LoadBalancingStrategy::ROUND_ROBIN);

  register_3_client_proxy(forward_management);

  // Save records
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(
      client_proxy1_, 1, client_proxy_1_1_requst_id));
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(
      client_proxy2_, 1, client_proxy_2_1_requst_id));
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(
      client_proxy1_, 2, client_proxy_1_2_requst_id));
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(
      client_proxy1_, 3, client_proxy_1_3_requst_id));
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(
      client_proxy2_, 2, client_proxy_2_2_requst_id));

  // Get client proxy 1 with sequence 2
  auto get_request_id =
    forward_management.get_request_info_from_corresponding_table(client_proxy1_, 2);
  ASSERT_TRUE(get_request_id.has_value());
  EXPECT_EQ(get_request_id.value(), client_proxy_1_2_requst_id);

  // Get client proxy 2 with sequence 1
  get_request_id =
    forward_management.get_request_info_from_corresponding_table(client_proxy2_, 1);
  ASSERT_TRUE(get_request_id.has_value());
  EXPECT_EQ(get_request_id.value(), client_proxy_2_1_requst_id);

  // Retrieve a non-existent record.
  get_request_id =
    forward_management.get_request_info_from_corresponding_table(client_proxy2_, 3);
  EXPECT_FALSE(get_request_id.has_value());
}

TEST_F(TestForwardManagement, test_request_client_proxy_with_ROUND_ROBIN)
{
  ForwardManagement forward_management(LoadBalancingStrategy::ROUND_ROBIN);

  register_3_client_proxy(forward_management);

  std::vector<std::optional<ForwardManagement::SharedClientProxy>> request_client_list;

  // Request the client proxy 3 times.
  for (int i = 0; i < 3; i++) {
    request_client_list.emplace_back(forward_management.request_client_proxy());
  }

  // Registered 3 client proxies, so each client proxy should get 1.
  int client_proxy1_count = 0;
  int client_proxy2_count = 0;
  int client_proxy3_count = 0;
  for (auto & client_proxy: request_client_list) {
    ASSERT_TRUE(client_proxy.has_value());
    if (client_proxy.value() == client_proxy1_) {
      client_proxy1_count++;
      EXPECT_TRUE(client_proxy1_count == 1);
    } else if (client_proxy.value() == client_proxy2_) {
      client_proxy2_count++;
      EXPECT_TRUE(client_proxy2_count == 1);
    } else if (client_proxy.value() == client_proxy3_) {
      client_proxy3_count++;
      EXPECT_TRUE(client_proxy3_count == 1);
    } else {
      FAIL() << "Found unknown client proxy";
    }
  }

  // Request the client proxy 3 times again.
  for (int i = 0; i < 3; i++) {
    request_client_list.emplace_back(forward_management.request_client_proxy());
  }

  // The order for obtaining client proxies should be the same as last time
  ASSERT_TRUE(request_client_list[3].has_value());
  EXPECT_TRUE(request_client_list[3].value() == request_client_list[0].value());
  ASSERT_TRUE(request_client_list[4].has_value());
  EXPECT_TRUE(request_client_list[4].value() == request_client_list[1].value());
  ASSERT_TRUE(request_client_list[5].has_value());
  EXPECT_TRUE(request_client_list[5].value() == request_client_list[2].value());
}

TEST_F(TestForwardManagement, test_request_client_proxy_with_LESS_REQUESTS)
{

  ForwardManagement forward_management(LoadBalancingStrategy::LESS_REQUESTS);

  register_3_client_proxy(forward_management);

  // For testing, use the same Request ID.
  ForwardManagement::SharedRequestID requst_id = std::make_shared<rmw_request_id_t>();

  // Save sending request info
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy1_, 1, requst_id));
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy2_, 1, requst_id));
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy3_, 1, requst_id));
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy1_, 2, requst_id));
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy3_, 2, requst_id));

  // Now should choose client proxy2
  auto get_client_proxy = forward_management.request_client_proxy();
  ASSERT_TRUE(get_client_proxy.has_value());
  EXPECT_TRUE(get_client_proxy.value() == client_proxy2_);

  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy2_, 2, requst_id));

  // The setup with the request of client proxy is complete.
  auto request_id =
    forward_management.get_request_info_from_corresponding_table(client_proxy1_, 1);
  ASSERT_TRUE(request_id.has_value());

  // Now should choose client proxy1
  get_client_proxy = forward_management.request_client_proxy();
  ASSERT_TRUE(get_client_proxy.has_value());
  EXPECT_TRUE(get_client_proxy.value() == client_proxy1_);
}

TEST_F(TestForwardManagement, test_request_client_proxy_with_LESS_RESPONSE_TIME)
{
  ForwardManagement forward_management(LoadBalancingStrategy::LESS_RESPONSE_TIME);

  register_3_client_proxy(forward_management);

  // For testing, use the same Request ID.
  ForwardManagement::SharedRequestID requst_id = std::make_shared<rmw_request_id_t>();

  // When a request is sent, add_one_record_to_corresponding_table() is called. When a response is
  // received, get_request_info_from_corresponding_table() is called. So, the time difference
  // between these calls is the time needed for the response.
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy1_, 1, requst_id));
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy2_, 1, requst_id));
  ASSERT_TRUE(
    forward_management.add_one_record_to_corresponding_table(client_proxy3_, 1, requst_id));

  std::this_thread::sleep_for(std::chrono::milliseconds(50));

  auto request_id =
    forward_management.get_request_info_from_corresponding_table(client_proxy3_, 1);
  ASSERT_TRUE(request_id.has_value());

  std::this_thread::sleep_for(std::chrono::milliseconds(10));

  request_id =
    forward_management.get_request_info_from_corresponding_table(client_proxy1_, 1);
  ASSERT_TRUE(request_id.has_value());

  std::this_thread::sleep_for(std::chrono::milliseconds(5));

  request_id =
    forward_management.get_request_info_from_corresponding_table(client_proxy2_, 1);
  ASSERT_TRUE(request_id.has_value());

  // The shortest response processing time should be client_proxy3
  // The next time you request, the client proxy should return client_proxy3.
  auto get_client_proxy = forward_management.request_client_proxy();
  ASSERT_TRUE(get_client_proxy.has_value());
  EXPECT_TRUE(get_client_proxy.value() == client_proxy3_);
}
