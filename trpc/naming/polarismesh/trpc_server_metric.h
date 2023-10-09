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

#include <string>

#include "polaris/plugin.h"

#include "trpc/common/config/trpc_config.h"

namespace trpc {

/// @brief Factory of reporting plug -ins on the TRPC monitoring of the polarismesh SDK
polaris::Plugin* TrpcServerMetricFactory();

/// @brief polarismesh SDK's TRPC monitoring plug -in
class TrpcServerMetric : public polaris::ServerMetric {
 public:
  polaris::ReturnCode Init(polaris::Config* config, polaris::Context* context) override;

  /// @brief Internal service call results report
  /// @param service_key Serve
  /// @param instance Instance
  /// @param ret_code Return code
  /// @param ret_status whether succeed
  /// @param delay Delay
  void MetricReport(const polaris::ServiceKey& service_key, const polaris::Instance& instance,
                    polaris::ReturnCode ret_code, polaris::CallRetStatus ret_status, uint64_t delay) override;

 private:
  bool enable_;
  std::string metrics_name_;
};

}  // namespace trpc
