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

#pragma once

#include <map>
#include <memory>
#include <string>
#include <unordered_map>

#include "polaris/context.h"
#include "polaris/provider.h"

#include "trpc/naming/common/common_defs.h"
#include "trpc/naming/polarismesh/common.h"
#include "trpc/naming/polarismesh/config/polarismesh_naming_conf.h"
#include "trpc/naming/registry.h"

namespace trpc {

/// @brief h SDK Heartbeat Reporting Asynchronous Return
class PolarisMeshHeartbeatCallback : public polaris::ProviderCallback {
 public:
  explicit PolarisMeshHeartbeatCallback(polaris::ServiceKey& service_key) : service_key_(service_key) {}

  void Response(polaris::ReturnCode code, const std::string& message) override {
    if (code != polaris::ReturnCode::kReturnOk) {
      TRPC_FMT_WARN(
          "AsyncHeartBeat failed, sdk returnCode:{}, message:{}, service_name:{} "
          "service_namespace:{}",
          static_cast<int>(code), message, service_key_.name_, service_key_.namespace_);
    } else {
      TRPC_FMT_DEBUG(
          "AsyncHeartBeat success, sdk returnCode:{}, message:{}, service_name:{} "
          "service_namespace:{}",
          static_cast<int>(code), message, service_key_.name_, service_key_.namespace_);
    }
  }

 private:
  polaris::ServiceKey service_key_;
};

/// @brief polarismesh Service Registration Plug -in
class PolarisMeshRegistry : public Registry {
 public:
  /// @brief The name of the plugin
  std::string Name() const override { return kPolarisPluginName; }

  /// @brief Plug -in version
  std::string Version() const { return kPolarisSDKVersion; }

  /// @brief initialization
  int Init() noexcept override;

  /// @brief In the internal implementation of the plug -in, it needs to be used when the thread needs to be created.
  /// You can use this interface uniformly
  void Start() noexcept override {}

  /// @brief When there is a thread in the internal implementation of the plug -in, the interface of the stop thread
  /// needs to be implemented
  void Stop() noexcept override {}

  /// @brief Various resources to destroy specific plug -in
  void Destroy() noexcept override;

  /// @brief Service registration interface
  int Register(const RegistryInfo* info) override;

  /// @brief Service anti -registration interface
  int Unregister(const RegistryInfo* info) override;

  /// @brief Service heartbeat on the reporting interface
  int HeartBeat(const RegistryInfo* info) override;

  /// @brief Service Heartbeat on the interface
  Future<> AsyncHeartBeat(const RegistryInfo* info) override;

  /// @brief Setter function for plugin_config_
  void SetPluginConfig(const naming::PolarisMeshNamingConfig& config) {
    plugin_config_ = config;
  }

 protected:
  /// @brief Remry config to complete the token information
  int GetTokenFromRegistryConfig(PolarisMeshRegistryInfo& polarismesh_registry_info);

  /// @brief Make up META information from registry config
  void GetMetadataFromRegistryConfig(PolarisMeshRegistryInfo& polarismesh_registry_info);

  PolarisMeshRegistryInfo SetupPolarisMeshRegistryInfo(const RegistryInfo& info);

 private:
  bool init_{false};
  uint64_t heartbeat_interval_;
  uint64_t heartbeat_timeout_;
  naming::PolarisMeshNamingConfig plugin_config_;
  std::map<polaris::ServiceKey, trpc::naming::ServiceConfig> services_config_;
  std::unique_ptr<polaris::ProviderApi> provider_api_{nullptr};
};

using PolarisMeshRegistryPtr = RefPtr<PolarisMeshRegistry>;

}  // namespace trpc
