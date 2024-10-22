find_package(ament_cmake_gtest REQUIRED)
find_package(rclcpp REQUIRED)

# finding gtest once in the highest scope
# prevents finding it repeatedly in each local scope
ament_find_gtest()

ament_add_gtest(test_common
  test_common.cpp
  ../src/common.cpp
)
ament_target_dependencies(test_common rclcpp)

