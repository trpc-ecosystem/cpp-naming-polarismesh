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

#include "gtest/gtest.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/metrics/metrics_factory.h"
#include "trpc/metrics/trpc_metrics.h"

namespace trpc {

class TestTrpcMetrics : public trpc::Metrics {
 public:
  TestTrpcMetrics() = default;

  ~TestTrpcMetrics() override = default;

  PluginType Type() const override { return PluginType::kMetrics; }

  std::string Name() const override { return "test_trpc_metrics"; }

  std::string Version() const { return "0.0.1"; }

  int Init() noexcept override { return 0; }

  void Start() noexcept override {}

  void Stop() noexcept override {}

  void Destroy() noexcept override {}

  int ModuleReport(const ModuleMetricsInfo& info) override {
    EXPECT_EQ("127.0.0.1", info.infos.find("pIp")->second);
    EXPECT_EQ("10001", info.infos.find("pPort")->second);
    return 0;
  }

  int SingleAttrReport(const SingleAttrMetricsInfo* info) { return 0; }

  int MultiAttrReport(const MultiAttrMetricsInfo* info) { return 0; }

  using trpc::Metrics::ModuleReport;
  using trpc::Metrics::MultiAttrReport;
  using trpc::Metrics::SingleAttrReport;
};

// TRPC_PLUGIN_REGISTER_CLASS(MetricsClassRegistry, Metrics, TestTrpcMetrics, "test_trpc_metrics");

TEST(TrpcServerMetric, RunWithoutMetrics) {
  trpc::TrpcServerMetric trpc_server_metric;
  // Before initialization, call the monitoring function and return directly
  polaris::ServiceKey service_key;
  polaris::Instance instance;
  polaris::ReturnCode ret_code = polaris::ReturnCode::kReturnOk;
  polaris::CallRetStatus ret_status = polaris::CallRetStatus::kCallRetOk;
  uint64_t delay = 10;
  trpc_server_metric.MetricReport(service_key, instance, ret_code, ret_status, delay);

  std::string content = "{enable: true}";
  std::string err_msg;
  polaris::Config* config = polaris::Config::CreateFromString(content, err_msg);
  ASSERT_TRUE(nullptr != config);

  polaris::Context* context = nullptr;
  polaris::ReturnCode ret = trpc_server_metric.Init(config, context);
  ASSERT_EQ(polaris::ReturnCode::kReturnOk, ret);

  // When there is no monitoring plug -in, call the monitoring function directly to return
  trpc_server_metric.MetricReport(service_key, instance, ret_code, ret_status, delay);
}

TEST(TrpcServerMetric, RunWithBadMetrics) {
  std::string content = "{enable: true, metrics_name: test}";
  std::string err_msg;
  polaris::Config* config = polaris::Config::CreateFromString(content, err_msg);
  ASSERT_TRUE(nullptr != config);

  trpc::TrpcServerMetric trpc_server_metric;
  polaris::Context* context = nullptr;
  polaris::ReturnCode ret = trpc_server_metric.Init(config, context);
  ASSERT_EQ(polaris::ReturnCode::kReturnOk, ret);

  // When the corresponding monitoring plug -in is not registered, call the monitoring function directly returns
  polaris::ServiceKey service_key;
  polaris::Instance instance;
  polaris::ReturnCode ret_code = polaris::ReturnCode::kReturnOk;
  polaris::CallRetStatus ret_status = polaris::CallRetStatus::kCallRetOk;
  uint64_t delay = 10;
  trpc_server_metric.MetricReport(service_key, instance, ret_code, ret_status, delay);
}

TEST(TrpcServerMetric, RunWithRightMetrics) {
  int ret = TrpcConfig::GetInstance()->Init("./trpc/naming/polarismesh/testing/polaris_test.yaml");
  EXPECT_EQ(0, ret);

  std::string content = "{enable: true}";
  std::string err_msg;
  polaris::Config* config = polaris::Config::CreateFromString(content, err_msg);
  ASSERT_TRUE(nullptr != config);

  trpc::TrpcServerMetric trpc_server_metric;
  polaris::Context* context = nullptr;
  polaris::ReturnCode return_code = trpc_server_metric.Init(config, context);
  ASSERT_EQ(polaris::ReturnCode::kReturnOk, return_code);

  polaris::ServiceKey service_key;
  service_key.name_ = "test";
  service_key.namespace_ = "Test";
  polaris::Instance instance("1", "127.0.0.1", 10001, 1);
  polaris::ReturnCode ret_code = polaris::ReturnCode::kReturnOk;
  polaris::CallRetStatus ret_status = polaris::CallRetStatus::kCallRetOk;
  uint64_t delay = 10;
  trpc_server_metric.MetricReport(service_key, instance, ret_code, ret_status, delay);
  ret_status = polaris::CallRetStatus::kCallRetTimeout;
  trpc_server_metric.MetricReport(service_key, instance, ret_code, ret_status, delay);
  ret_status = polaris::CallRetStatus::kCallRetError;
  trpc_server_metric.MetricReport(service_key, instance, ret_code, ret_status, delay);
}

}  // namespace trpc
