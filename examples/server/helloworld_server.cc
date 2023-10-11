//
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 THL A29 Limited, a Tencent company.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
//

#include <memory>
#include <string>

#include "fmt/format.h"
#include "trpc/common/trpc_app.h"
#include "trpc/naming/registry_factory.h"

#include "examples/server/greeter_service.h"

#include "trpc/naming/polarismesh/polarismesh_registry_api.h"
#include "trpc/naming/polarismesh/polarismesh_selector_api.h"
#include "trpc/naming/polarismesh/polarismesh_limiter_api.h"
#include "trpc/naming/polarismesh/polarismesh_registry.h"

namespace test::helloworld {

class HelloWorldServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    const auto& config = ::trpc::TrpcConfig::GetInstance()->GetServerConfig();
    // Set the service name, which must be the same as the value of the `/server/service/name` configuration item
    // in the configuration file, otherwise the framework cannot receive requests normally.
    std::string service_name = fmt::format("{}.{}.{}.{}", "trpc", config.app, config.server, "Greeter");
    TRPC_FMT_INFO("service name:{}", service_name);
    RegisterService(service_name, std::make_shared<GreeterServiceImpl>());

    // If the trpc-cpp self-registration and heartbeat report are not enabled, you need to manually register and report the heartbeat
    register_info.name = service_name;
    register_info.host = config.services_config[0].ip;
    register_info.port = config.services_config[0].port;
    register_info.meta["namespace"] = trpc::TrpcConfig::GetInstance()->GetGlobalConfig().env_namespace;

    RegisterPolarismesh();
    HeartBeat();

    return 0;
  }

  void HeartBeat() {
    auto registry = trpc::static_pointer_cast<trpc::PolarisMeshRegistry>(trpc::RegistryFactory::GetInstance()->Get("polarismesh"));
    new std::thread([=] {
      while (true) {
        auto ret_code = registry->HeartBeat(&register_info);
        if (ret_code != 0) {
          TRPC_FMT_ERROR("instance heartbeat with error");
        } else {
          TRPC_FMT_DEBUG("instance heartbeat success");
        }
        sleep(3);
      }
    });
  }

  void RegisterPolarismesh() {
    auto registry = trpc::static_pointer_cast<trpc::PolarisMeshRegistry>(trpc::RegistryFactory::GetInstance()->Get("polarismesh"));
    auto ret = registry->Register(&register_info);
    if (ret != 0) {
      TRPC_FMT_ERROR("Failed to register service");
    } else {
      TRPC_FMT_INFO("Success to register service");
    }
  }

  void Destroy() override {}

  private:
   trpc::RegistryInfo register_info;
};

}  // namespace test::helloworld

int main(int argc, char** argv) {
  test::helloworld::HelloWorldServer helloworld_server;

  ::trpc::polarismesh::registry::Init();
  ::trpc::polarismesh::selector::Init();
  ::trpc::polarismesh::limiter::Init();

  helloworld_server.Main(argc, argv);
  helloworld_server.Wait();

  return 0;
}
