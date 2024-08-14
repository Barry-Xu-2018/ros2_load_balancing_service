cmake_minimum_required(VERSION 3.8)
project(load_balancing_service)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Default to C++17
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)
find_package(rosidl_typesupport_introspection_cpp REQUIRED)

add_executable(load_balancing_service
  src/service_client_proxy.cpp
  src/service_server_proxy.cpp
  src/services_manager.cpp
)
ament_target_dependencies(load_balancing_service rclcpp)

install(TARGETS
  load_balancing_service
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
