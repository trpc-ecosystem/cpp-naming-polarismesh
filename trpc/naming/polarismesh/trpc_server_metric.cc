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

#include "trpc/naming/polarismesh/trpc_server_metric.h"

#include <string>
#include <vector>

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/trpc_metrics.h"
#include "trpc/util/log/logging.h"

namespace trpc {

polaris::Plugin* TrpcServerMetricFactory() { return new trpc::TrpcServerMetric(); }

// Try to get the monitoring plug -in name in the frame configuration
static void FixMetricName(std::string& metrics_name) {
  // Try to use the first monitoring plug-in in the configuration file
  std::vector<std::string> plugin_names;
  bool exist = trpc::TrpcConfig::GetInstance()->GetPluginNodes("metrics", plugin_names);
  if (!exist) {
    TRPC_LOG_DEBUG("plugin not found in yaml: plugins->metrics, configure if you want to use.");
    return;
  }

  if (plugin_names.size() != 0) {
    metrics_name = plugin_names[0];
    return;
  }

  TRPC_FMT_WARN("no trpc metric plugin can be used");
  return;
}

polaris::ReturnCode TrpcServerMetric::Init(polaris::Config* config, polaris::Context* context) {
  enable_ = config->GetBoolOrDefault("enable", false);
  metrics_name_ = config->GetStringOrDefault("metrics_name", "");

  if (enable_ && metrics_name_.empty()) {
    FixMetricName(metrics_name_);
  }

  return polaris::ReturnCode::kReturnOk;
}

void TrpcServerMetric::MetricReport(const polaris::ServiceKey& service_key, const polaris::Instance& instance,
                                    polaris::ReturnCode ret_code, polaris::CallRetStatus ret_status, uint64_t delay) {
  if (!enable_ || metrics_name_.empty()) {
    return;
  }
  return;
}

}  // namespace trpc
