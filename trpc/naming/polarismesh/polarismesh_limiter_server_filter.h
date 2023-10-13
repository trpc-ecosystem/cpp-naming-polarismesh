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

#include "trpc/common/config/trpc_config.h"
#include "trpc/filter/filter.h"
#include "trpc/naming/limiter.h"
#include "trpc/naming/limiter_factory.h"
#include "trpc/naming/polarismesh/config/polarismesh_naming_conf.h"
#include "trpc/server/server_context.h"

namespace trpc {

/// @brief polarismesh server limited flower Filter
class PolarisMeshLimiterServerFilter : public MessageServerFilter {
 public:
  PolarisMeshLimiterServerFilter() = default;

  ~PolarisMeshLimiterServerFilter() override = default;

  std::string Name() override { return "polarismesh_limiter"; }

  int Init();

  /// @brief Get a buried point
  std::vector<FilterPoint> GetFilterPoint() override;

  /// @brief Trigger the corresponding treatment at the buried point
  void operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) override;

 private:
  // Determine whether it is restricted
  LimitRetCode ShouldLimit(const ServerContextPtr& context);

  // Reporting
  void FinishLimit(const ServerContextPtr& context, LimitRetCode ret_code);

 private:
  // Streaming plug -in object
  LimiterPtr limiter_;

  // Do you need to report the call
  bool update_call_result_ = false;
};

using PolarisMeshLimiterServerFilterPtr = RefPtr<PolarisMeshLimiterServerFilter>;

}  // namespace trpc
