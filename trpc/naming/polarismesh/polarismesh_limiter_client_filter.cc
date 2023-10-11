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

#include "trpc/naming/polarismesh/polarismesh_limiter_client_filter.h"

#include <utility>

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/common/status.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"

namespace trpc {

LimitRetCode PolarisMeshLimiterClientFilter::ShouldLimit(const ClientContextPtr& context) {
  // Fill limit info
  LimitInfo limit_info;
  limit_info.name = context->GetServiceProxyOption()->target;            // Join the service name
  limit_info.name_space = context->GetServiceProxyOption()->name_space;  // The named space
  // Limat Label introduced in the framework is Method, Caller
  limit_info.labels.insert(std::make_pair("method", context->GetFuncName()));    // Method name
  limit_info.labels.insert(std::make_pair("caller", context->GetCallerName()));  // Main service name

  return limiter_->ShouldLimit(&limit_info);
}

void PolarisMeshLimiterClientFilter::FinishLimit(const ClientContextPtr& context, LimitRetCode ret_code) {
  if (!update_call_result_) {
    return;
  }

  LimitResult limit_result;
  limit_result.name = context->GetServiceProxyOption()->target;                    // Join the service name
  limit_result.name_space = context->GetServiceProxyOption()->name_space;          // Turned naming space
  limit_result.labels.insert(std::make_pair("method", context->GetFuncName()));    // Method name
  limit_result.labels.insert(std::make_pair("caller", context->GetCallerName()));  // Main service name
  limit_result.limit_ret_code = ret_code;
  if (ret_code == LimitRetCode::kLimitOK) {
    limit_result.framework_result = context->GetStatus().GetFrameworkRetCode();
    limit_result.interface_result = context->GetStatus().GetFuncRetCode();
    limit_result.cost_time = (trpc::time::GetMicroSeconds() - context->GetSendTimestampUs()) / 1000;  // ms
  }

  limiter_->FinishLimit(&limit_result);
}

std::vector<FilterPoint> PolarisMeshLimiterClientFilter::GetFilterPoint() {
  std::vector<FilterPoint> points = {FilterPoint::CLIENT_PRE_RPC_INVOKE, FilterPoint::CLIENT_POST_RPC_INVOKE};
  return points;
}

void PolarisMeshLimiterClientFilter::operator()(FilterStatus& status, FilterPoint point, const ClientContextPtr& context) {
  TRPC_ASSERT(context->GetServiceProxyOption() && "service option is null");
  TRPC_ASSERT(limiter_ && "limiter is null");
  if (point == FilterPoint::CLIENT_PRE_RPC_INVOKE) {
    LimitRetCode ret_code = ShouldLimit(context);
    if (ret_code == LimitRetCode::kLimitReject) {
      std::string error = "Client limit reject of " + context->GetServiceProxyOption()->target;
      TRPC_LOG_ERROR(error);
      context->SetStatus(Status(TrpcRetCode::TRPC_CLIENT_LIMITED_ERR, 0, std::move(error)));
      FinishLimit(context, ret_code);
      status = FilterStatus::REJECT;
      return;
    }
  } else if (point == FilterPoint::CLIENT_POST_RPC_INVOKE) {
    FinishLimit(context, LimitRetCode::kLimitOK);
  }

  status = FilterStatus::CONTINUE;
}

}  // namespace trpc
