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

#ifndef SERVICE_SERVER_PROXY_HPP_
#define SERVICE_SERVER_PROXY_HPP_

#include <memory>
#include <string>


class ServiceServerProxy : public std::enable_shared_from_this<ServiceServerProxy>
{
public:
  ServiceServerProxy(std::string service_server_name);
  ~ServiceServerProxy();
};

#endif  // SERVICE_SERVER_PROXY_HPP_
