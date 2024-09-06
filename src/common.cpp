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

#include <regex>

#include "rclcpp/logging.hpp"

#include "common.hpp"

bool
is_load_balancing_service(const std::string & base_service_name, const std::string & service_name)
{
  if (base_service_name.empty()) {
    RCLCPP_ERROR(rclcpp::get_logger("common"), "base_service_name is empty.");
    return false;
  }

  if (service_name.empty()) {
    RCLCPP_ERROR(rclcpp::get_logger("common"), "service_name is empty.");
    return false;
  }

  std::regex pattern(".*/" + PREFIX_LOAD_BALANCING + "/" + base_service_name + "/.{1,}");
  if (std::regex_search(service_name, pattern)) {
    return true;
  }

  return false;
}