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

#include "trpc/naming/polarismesh/polaris_limiter_server_filter.h"

#include <utility>

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/common/status.h"
#include "trpc/server/service.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/time.h"
namespace trpc {

int PolarisLimiterServerFilter::Init() {
  limiter_ = LimiterFactory::GetInstance()->Get("polarismesh");
  trpc::naming::RateLimiterConfig config;
  if (TrpcConfig::GetInstance()->GetPluginConfig<trpc::naming::RateLimiterConfig>("limiter", "polarismesh", config)) {
    update_call_result_ = config.update_call_result;
  }
  return 0;
}

LimitRetCode PolarisLimiterServerFilter::ShouldLimit(const ServerContextPtr& context) {
  LimitInfo limit_info;
  limit_info.name = context->GetService()->GetName();                                  // Join the service name
  limit_info.name_space = TrpcConfig::GetInstance()->GetGlobalConfig().env_namespace;  // The named space

  // Limat Label introduced in the framework is Method, Caller
  const auto& func_name = context->GetFuncName();
  std::string::size_type found = func_name.rfind('/');
  if (found != std::string::npos) {
    std::string sub_func_name(func_name, found + 1);
    limit_info.labels.insert(std::make_pair("method", sub_func_name));  // Take the last paragraph of the method name
  } else {
    limit_info.labels.insert(std::make_pair("method", func_name));  // Method name
  }

  limit_info.labels.insert(std::make_pair("caller", context->GetCallerName()));  // Main service name

  LimitRetCode ret_code = limiter_->ShouldLimit(&limit_info);

  return ret_code;
}

void PolarisLimiterServerFilter::FinishLimit(const ServerContextPtr& context, LimitRetCode ret_code) {
  if (!update_call_result_) {
    return;
  }

  LimitResult limit_result;
  limit_result.name = context->GetService()->GetName();                                  // Join the service name
  limit_result.name_space = TrpcConfig::GetInstance()->GetGlobalConfig().env_namespace;  // The named space
  limit_result.labels.insert(std::make_pair("method", context->GetFuncName()));          // Method name
  limit_result.labels.insert(std::make_pair("caller", context->GetCallerName()));        // Main service name
  limit_result.limit_ret_code = ret_code;
  if (ret_code == LimitRetCode::kLimitOK) {
    limit_result.framework_result = context->GetStatus().GetFrameworkRetCode();
    limit_result.interface_result = context->GetStatus().GetFuncRetCode();
    limit_result.cost_time = trpc::time::GetMilliSeconds() - context->GetRecvTimestamp();
  }

  limiter_->FinishLimit(&limit_result);
}

std::vector<FilterPoint> PolarisLimiterServerFilter::GetFilterPoint() {
  std::vector<FilterPoint> points = {FilterPoint::SERVER_PRE_RPC_INVOKE, FilterPoint::SERVER_POST_RPC_INVOKE};
  return points;
}

void PolarisLimiterServerFilter::operator()(FilterStatus& status, FilterPoint point, const ServerContextPtr& context) {
  TRPC_ASSERT(context->GetService() && "service adapter is null");
  TRPC_ASSERT(limiter_ && "limiter is null");
  if (point == FilterPoint::SERVER_PRE_RPC_INVOKE) {
    LimitRetCode ret_code = ShouldLimit(context);
    if (ret_code == LimitRetCode::kLimitReject) {
      std::string error = "Server limit reject of " + context->GetService()->GetName();
      TRPC_LOG_ERROR(error);
      context->SetStatus(Status(TrpcRetCode::TRPC_SERVER_LIMITED_ERR, 0, std::move(error)));
      FinishLimit(context, ret_code);
      status = FilterStatus::REJECT;
      return;
    }
  } else if (point == FilterPoint::SERVER_POST_RPC_INVOKE) {
    FinishLimit(context, LimitRetCode::kLimitOK);
  }

  status = FilterStatus::CONTINUE;
}

}  // namespace trpc
