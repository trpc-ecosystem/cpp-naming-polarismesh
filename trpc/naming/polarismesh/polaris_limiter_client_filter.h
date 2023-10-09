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

#include <memory>
#include <string>
#include <vector>

#include "trpc/client/client_context.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter.h"
#include "trpc/naming/limiter.h"
#include "trpc/naming/limiter_factory.h"
#include "trpc/naming/polarismesh/config/polaris_naming_conf.h"

namespace trpc {

/// @brief polarismesh Client Limitor Filter
class PolarisLimiterClientFilter : public MessageClientFilter {
 public:
  PolarisLimiterClientFilter() = default;

  ~PolarisLimiterClientFilter() override = default;

  std::string Name() override { return "polaris_limiter"; }

  int Init() override {
    limiter_ = LimiterFactory::GetInstance()->Get("polarismesh");

    trpc::naming::RateLimiterConfig config;
    if (TrpcConfig::GetInstance()->GetPluginConfig<trpc::naming::RateLimiterConfig>("limiter", "polarismesh", config)) {
      update_call_result_ = config.update_call_result;
    }

    return 0;
  }

  /// @brief Get a buried point
  std::vector<FilterPoint> GetFilterPoint() override;

  /// @brief Trigger the corresponding treatment at the buried point
  void operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) override;

 private:
  // Determine whether it is restricted
  LimitRetCode ShouldLimit(const ClientContextPtr& context);

  // Reporting
  void FinishLimit(const ClientContextPtr& context, LimitRetCode ret_code);

 private:
  // Streaming plug -in object
  LimiterPtr limiter_;

  // Do you need to report the call
  bool update_call_result_ = false;
};

using PolarisLimiterClientFilterPtr = RefPtr<PolarisLimiterClientFilter>;

}  // namespace trpc
