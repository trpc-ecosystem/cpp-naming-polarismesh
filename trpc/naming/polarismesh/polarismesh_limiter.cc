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

#include "trpc/naming/polarismesh/polarismesh_limiter.h"

#include <any>
#include <utility>
#include <vector>

#include "polaris/context.h"

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/naming/polarismesh/config/polarismesh_naming_conf.h"
#include "trpc/naming/polarismesh/trpc_share_context.h"

namespace trpc {

int PolarisMeshLimiter::Init() noexcept {
  if (init_) {
    TRPC_FMT_DEBUG("Already init");
    return 0;
  }

  if (plugin_config_.name.empty()) {
    trpc::naming::PolarisMeshNamingConfig config;
    SetPolarisMeshSelectorConf(config);
    plugin_config_ = config;
  }

  auto config = std::any_cast<trpc::naming::PolarisMeshNamingConfig>(plugin_config_);
  if (trpc::TrpcShareContext::GetInstance()->Init(config) != 0) {
    return -1;
  }

  auto context = trpc::TrpcShareContext::GetInstance()->GetPolarisContext();
  limit_api_ = std::unique_ptr<polaris::LimitApi>(polaris::LimitApi::Create(context.get()));
  if (!limit_api_) {
    TRPC_FMT_ERROR("Create LimitApi failed");
    return -1;
  }

  init_ = true;
  return 0;
}

void PolarisMeshLimiter::Destroy() noexcept {
  if (!init_) {
    TRPC_FMT_DEBUG("No init yet");
    return;
  }

  limit_api_ = nullptr;
  trpc::TrpcShareContext::GetInstance()->Destroy();
  init_ = false;
  return;
}

// Simultaneously obtain a current -limiting state interface
LimitRetCode PolarisMeshLimiter::ShouldLimit(const LimitInfo* info) {
  if (!init_) {
    TRPC_FMT_ERROR("No init yet");
    return LimitRetCode::kLimitError;
  }

  if (info->name.empty() || info->name_space.empty()) {
    TRPC_FMT_ERROR("Invalid input parameter");
    return LimitRetCode::kLimitError;
  }

  polaris::QuotaRequest quota_request;
  quota_request.SetServiceName(info->name);
  quota_request.SetServiceNamespace(info->name_space);
  if (!info->labels.empty()) {
    quota_request.SetLabels(info->labels);
  }
  quota_request.SetTimeout(plugin_config_.ratelimiter_config.timeout);

  polaris::QuotaResponse* quota_response = nullptr;
  polaris::ReturnCode ret = limit_api_->GetQuota(quota_request, quota_response);
  if (ret != polaris::kReturnOk) {
    TRPC_FMT_ERROR("GetQuota failed, sdk returnCode:{}, service_name:{}, service_namespace{}",
                   static_cast<int32_t>(ret), info->name, info->name_space);
    if (quota_response != nullptr) {
      delete quota_response;
    }
    return LimitRetCode::kLimitError;
  }

  TRPC_ASSERT(quota_response != nullptr && "GetQuota success, but QuotaResponse is nullptr");

  LimitRetCode retcode;
  if (quota_response->GetResultCode() == polaris::kQuotaResultOk) {
    // The request is not restricted, and you can continue to execute
    retcode = LimitRetCode::kLimitOK;
  } else {
    // Request being restricted
    retcode = LimitRetCode::kLimitReject;
  }

  delete quota_response;
  return retcode;
}

polaris::LimitCallResultType GetCallResultType(trpc::LimitRetCode limit_ret_code, int call_ret) {
  if (trpc::LimitRetCode::kLimitReject == limit_ret_code) {
    return polaris::LimitCallResultType::kLimitCallResultLimited;
  }

  if (TrpcRetCode::TRPC_INVOKE_SUCCESS == call_ret) {
    return polaris::LimitCallResultType::kLimitCallResultOk;
  }

  return polaris::LimitCallResultType::kLimitCallResultFailed;
}

int PolarisMeshLimiter::FinishLimit(const LimitResult* result) {
  // Extension function: Automatically report the current limit result for the dynamic threshold adjustment of the polar
  // star for the current limit rule You need to configure the UpdateCallResult in the configuration to true
  if (!plugin_config_.ratelimiter_config.update_call_result) {
    return 0;
  }

  // ShouldLimit返回kLimitOK或者kLimitReject时才需要上报
  if (result->limit_ret_code == trpc::LimitRetCode::kLimitError) {
    return 0;
  }

  return 0;
}

}  // namespace trpc
