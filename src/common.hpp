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

#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <string>

// service name for load balancing
// The format:
// /NAMESPACE/PREFIX_LOAD_BALANCING/service_name/USER_INDEX
const std::string PREFIX_LOAD_BALANCING = "load_balancing";

/**
 * @brief Determine if a service name belongs to a load balancing service.
 *
 * @param base_service_name original service name
 * @param service_name a service name to be checked
 * @return True if The service name meets the naming conventions of a load balancing service.FORWARD_MANAGEMENT
 *   otherwise False.
 */
bool
is_load_balancing_service(const std::string & base_service_name, const std::string & service_name);

#endif  // COMMON_HPP_
