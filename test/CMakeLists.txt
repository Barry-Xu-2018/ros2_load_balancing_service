find_package(ament_cmake_gtest REQUIRED)
find_package(rclcpp REQUIRED)
find_package(test_msgs REQUIRED)

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
ament_target_dependencies(test_forward_management rclcpp test_msgs)
