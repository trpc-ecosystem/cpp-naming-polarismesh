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

#include "trpc/naming/polarismesh/trpc_share_context.h"

#include <string>

#include "polaris/context/context_impl.h"
#include "polaris/log.h"

#include "trpc/naming/polarismesh/trpc_server_metric.h"
#include "trpc/util/log/logging.h"

namespace trpc {

int TrpcShareContext::Init(const trpc::naming::PolarisNamingConfig& config) {
  std::lock_guard<std::mutex> lock(mutex_);
  if (init_) {
    TRPC_FMT_DEBUG("Already init");
    return 0;
  }
  std::string err_msg;
  std::shared_ptr<polaris::Config> polaris_config(
      polaris::Config::CreateFromString(config.orig_selector_config, err_msg));
  if (!polaris_config) {
    TRPC_FMT_ERROR("Create polarismesh config failed, error:{}", err_msg);
    return -1;
  }

  // Register the polarismesh monitoring plugin
  polaris::RegisterPlugin("trpc", polaris::kPluginServerMetric, trpc::TrpcServerMetricFactory);

  // Initialize the polarismesh Context
  polaris_context_ = std::shared_ptr<polaris::Context>(
      polaris::Context::Create(polaris_config.get(), polaris::ContextMode::kShareContext));
  if (!polaris_context_) {
    TRPC_FMT_ERROR("Create polarismesh context failed");
    return -1;
  }

  init_ = true;
  return 0;
}

void TrpcShareContext::Destroy() {
  std::lock_guard<std::mutex> lock(mutex_);
  if (!init_) {
    TRPC_FMT_DEBUG("No init yet");
    return;
  }

  polaris_context_ = nullptr;
  init_ = false;
}

polaris::ServerConnector* TrpcShareContext::GetServerConnector() {
  if (polaris_context_) {
    auto server_connector = polaris_context_->GetContextImpl()->GetServerConnector();
    if (server_connector) {
      polaris::ServerConnector* p = dynamic_cast<polaris::ServerConnector*>(server_connector);
      return p;
    }
  }

  TRPC_FMT_ERROR("Get Server connector failed");
  return nullptr;
}

}  // namespace trpc
