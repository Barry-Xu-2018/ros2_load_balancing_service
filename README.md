# ros2_load_balancing_service (In development)
This package helps ROS2 services implement load balancing functionality.

# How to build load balancing service application
In ROS2 environment (such as Jazzy, rolling)

Use rolling as example
```bash
$ source /opt/ros/rolling/setup.bash
$ mkdir -p ros2_ws/src && cd ros2_ws/src
$ git clone https://github.com/Barry-Xu-2018/ros2_load_balancing_service.git
$ cd ..
$ colcon build
```

# How to run

Use `add_two_ints_server` (service server) and `add_two_ints_client_async` (service client) as an example to introduce how to use load balancing service application

## launch Load balancing service application
Load balancing service application needs to run first.
```bash
$ source /PATH/TO/ros2_ws/install/setup.bash
$ ros2 run load_balancing_service load_balancing_service -s add_two_ints -t example_interfaces/srv/AddTwoInts --strategy less_requests -i 1
```
The application parameters are introduced as follows
```
Usage:
    /root/ros2_ws/install/load_balancing_service/lib/load_balancing_service/load_balancing_service [-h|--help] [-s|--service-name SERVICE_NAME] [-t|--service-type SERVICE_TYPE] [--strategy XXX] [-i|--interval TIME]
       --strategy choose one of "round_robin", "less_requests" and "less_response_time"
                  If not set, "round_robin" is used as default.
                  "round_robin": select the service server in order.
                  "less_requests": select the service server with the fewest requests.
                  "less_response_time": select the service server with the shortest average response time.
       --interval Interval to discovery service servers. Unit is second.
                  If not set, default is 1s.
```

After run, you will get the below output
```
   Load balancing service name: /load_balancing/add_two_ints
                  Service type: example_interfaces/srv/AddTwoInts
       Load balancing strategy: less_requests
  Interval of server discovery: 1s
------------------------------
Service client remap service name to /load_balancing/add_two_ints
Service server remap service name to /load_balancing/add_two_ints/XXX
------------------------------
```

## launch service server

The service server can refer to the above hint in log output to remap the service name for using the load balancing service.  
Assume we start with 2 service servers. Of course, you can add service servers at any time.  
In a new terminal,
```bash
$ source /opt/ros/rolling/setup.bash
$ ros2 run demo_nodes_cpp add_two_ints_server --ros-args -r add_two_ints:=load_balancing/add_two_ints/s1
```
In another new terminal,
```bash
$ source /opt/ros/rolling/setup.bash
$ ros2 run demo_nodes_cpp add_two_ints_server --ros-args -r add_two_ints:=load_balancing/add_two_ints/s2
```
Note that you must use different name under load_balancing/add_two_ints/.  


## launch service client

We can run any number of service clients to connect to the service.
```bash
$ source /opt/ros/rolling/setup.bash
$ ros2 run demo_nodes_cpp add_two_ints_client_async --ros-args -r add_two_ints:=load_balancing/add_two_ints
```
