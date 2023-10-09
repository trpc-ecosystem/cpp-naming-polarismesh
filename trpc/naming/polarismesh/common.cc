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

#include "trpc/naming/polarismesh/common.h"

#include <iostream>
#include <sstream>
#include <string>
#include <utility>

#include "yaml-cpp/yaml.h"

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/util/log/logging.h"

namespace {

bool InCircuitBreakWhiteList(trpc::ReadersWriterData<std::set<int>>& whitelist, int framework_ret) {
  auto& reader = whitelist.Reader();
  if (reader.find(framework_ret) != reader.end()) {
    return true;
  }

  return false;
}

}  // namespace

namespace trpc {

void SetPolarisSelectorConf(trpc::naming::PolarisNamingConfig& config) {
  if (!trpc::TrpcConfig::GetInstance()->GetPluginConfig<trpc::naming::SelectorConfig>("selector", "polarismesh",
                                                                                      config.selector_config)) {
    TRPC_FMT_ERROR("get selector polarismesh config error, use default value");
  }

  bool enable_limiter = true;
  if (!TrpcConfig::GetInstance()->GetPluginConfig<trpc::naming::RateLimiterConfig>("limiter", "polarismesh",
                                                                                   config.ratelimiter_config)) {
    TRPC_FMT_DEBUG("get polarismesh limiter config failed, so disable it");
    enable_limiter = false;
  }

  // Compatible with the old version logic: If the business is not set to a local IP, it will be passed into the
  // local_ip in the configuration file Global
  auto& bind_ip = config.selector_config.global_config.api_config.bind_ip;
  if (bind_ip.empty()) {
    std::string local_ip = trpc::TrpcConfig::GetInstance()->GetGlobalConfig().local_ip;
    if (!local_ip.empty() && local_ip.compare("0.0.0.0")) {
      bind_ip = local_ip;
    }
  }

  // Constructing polarismesh Configuration File Format
  YAML::Node node_consumer(config.selector_config);
  std::stringstream strstream;
  strstream << node_consumer;
  std::string orig_selector_config = strstream.str();

  config.orig_selector_config = orig_selector_config;

  // When configured
  if (enable_limiter) {
    // add orig_ratelimiter_config
    YAML::Node node_ratelimiter;
    node_ratelimiter["rateLimiter"]["mode"] = config.ratelimiter_config.mode;
    node_ratelimiter["rateLimiter"]["rateLimitCluster"] = config.ratelimiter_config.cluster_config;

    strstream.str("");
    strstream << std::endl << node_ratelimiter;
    std::string orig_ratelimiter_config = strstream.str();

    config.orig_selector_config += orig_ratelimiter_config;
  }
}

polaris::CallRetStatus FrameworkRetToPolarisRet(ReadersWriterData<std::set<int>>& whitelist, int framework_ret) {
  if (framework_ret == TrpcRetCode::TRPC_INVOKE_SUCCESS || InCircuitBreakWhiteList(whitelist, framework_ret)) {
    return polaris::CallRetStatus::kCallRetOk;
  } else if (framework_ret == TrpcRetCode::TRPC_CLIENT_CONNECT_ERR ||
             framework_ret == TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR ||
             framework_ret == TrpcRetCode::TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR) {
    return polaris::CallRetStatus::kCallRetTimeout;
  } else {
    return polaris::CallRetStatus::kCallRetError;
  }
}

}  // namespace trpc
