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

#include "../src/common.hpp"

TEST(TestCommonFunctionality, test_is_load_balancing_service)
{
  const std::string service_base = "test_service";
  std::vector<std::pair<std::string, bool>> test_list = {
    {"/test_service", false},
    {"/atest_service", false},
    {"/test_service1", false},
    {"/load_balancing/test_service", false},
    {"/abc/load_balancing/test_service", false},
    {"/load_balancing/test_service/1", true},
    {"/load_balancing/test_service/srv1", true},
    {"/abc/load_balancing/test_service/2", true},
    {"/abc/load_balancing/test_service/srv2", true},
    {"/abc/load_balancing/test_service/1", true},
  };

  for (auto & [service_name, expected_result] : test_list) {
    EXPECT_EQ(is_load_balancing_service(service_base, service_name), expected_result);
  }
}
