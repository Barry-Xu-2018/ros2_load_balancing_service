find_package(ament_cmake_gtest REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_srvs REQUIRED)
find_package(example_interfaces REQUIRED)

# finding gtest once in the highest scope
# prevents finding it repeatedly in each local scope
ament_find_gtest()

ament_add_gtest(test_common
  test_common.cpp
  ../src/common.cpp
)
ament_target_dependencies(test_common rclcpp)

ament_add_gtest(test_data_queue
  test_data_queue.cpp
)
ament_target_dependencies(test_data_queue rclcpp)

ament_add_gtest(test_forward_management
  test_forward_management.cpp
  ../src/forward_management.cpp
)
ament_target_dependencies(test_forward_management rclcpp std_srvs)

ament_add_gtest(test_service_server_proxy
  test_service_server_proxy.cpp
  ../src/service_server_proxy.cpp
)
ament_target_dependencies(test_service_server_proxy rclcpp std_srvs)

ament_add_gtest(test_service_client_proxy_manager
  test_service_client_proxy_manager.cpp
  ../src/common.cpp
  ../src/service_client_proxy_manager.cpp
)
ament_target_dependencies(test_service_client_proxy_manager rclcpp std_srvs)

ament_add_gtest(test_message_forward_process
  test_message_forward_process.cpp
  ../src/common.cpp
  ../src/forward_management.cpp
  ../src/service_client_proxy_manager.cpp
  ../src/service_server_proxy.cpp
  ../src/message_forward_process.cpp
)
ament_target_dependencies(test_message_forward_process rclcpp example_interfaces)
