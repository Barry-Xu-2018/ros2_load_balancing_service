# ros2_load_balancing_service (In development)
This package helps ROS2 services implement load balancing functionality.

This package relies on ROS2 to support the service server/service client in retrieving serialized data. Currently, there is `rclcpp::GenericClient`, but this class only supports sending serialized request data, while the received data is still deserialized response data. 
Before implementing this package, I will first implement the dependent interfaces in ROS2.
