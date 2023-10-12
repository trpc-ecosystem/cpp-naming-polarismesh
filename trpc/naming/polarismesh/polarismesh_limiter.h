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

#include <any>
#include <memory>
#include <string>

#include "polaris/limit.h"

#include "trpc/naming/limiter.h"
#include "trpc/naming/polarismesh/common.h"
#include "trpc/naming/polarismesh/config/polarismesh_naming_conf.h"

namespace trpc {

/// @brief polarismesh rate limiting plugin
class PolarisMeshLimiter : public Limiter {
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

  /// @brief Get access to the current -limiting state interface simultaneously
  LimitRetCode ShouldLimit(const LimitInfo* info) override;

  /// @brief End when the current limit processing is called to report some current limit data, etc.
  int FinishLimit(const LimitResult* result) override;

  /// @brief Setter function for plugin_config_
  void SetPluginConfig(const naming::PolarisMeshNamingConfig& config) {
    plugin_config_ = config;
  }

 private:
  bool init_{false};
  naming::PolarisMeshNamingConfig plugin_config_;
  std::unique_ptr<polaris::LimitApi> limit_api_{nullptr};
};

using PolarisMeshLimiterPtr = RefPtr<PolarisMeshLimiter>;

}  // namespace trpc
