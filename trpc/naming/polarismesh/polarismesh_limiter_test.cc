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

#include <pthread.h>
#include <stdint.h>

#include <memory>
#include <string>
#include <utility>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "polaris/model/constants.h"
#include "polaris/plugin/service_router/set_division_router.h"
#include "polaris/utils/string_utils.h"
#include "yaml-cpp/yaml.h"

#include "trpc/common/trpc_plugin.h"
#include "trpc/naming/limiter_factory.h"
#include "trpc/naming/polarismesh/mock_polarismesh_api_test.h"

namespace trpc {

class PolarisLimitTest : public polaris::MockServerConnectorTest {
 protected:
  virtual void SetUp() {
    MockServerConnectorTest::SetUp();
    ASSERT_TRUE(polaris::TestUtils::CreateTempDir(persist_dir_));
    InitPolarisMeshLimiter();
    InitServiceNormalData();
  }

  virtual void TearDown() {
    trpc::LimiterFactory::GetInstance()->Get("polarismesh")->Destroy();
    trpc::TrpcPlugin::GetInstance()->UnregisterPlugins();
    EXPECT_TRUE(polaris::TestUtils::RemoveDir(persist_dir_));
    for (std::size_t i = 0; i < event_thread_list_.size(); ++i) {
      pthread_join(event_thread_list_[i], NULL);
    }
    MockServerConnectorTest::TearDown();
  }

  void InitPolarisMeshLimiter() {
    PolarisNamingTestConfigSwitch default_Switch;
    default_Switch.need_ratelimiter = true;
    YAML::Node root = YAML::Load(trpc::buildPolarisMeshNamingConfig(default_Switch));
    YAML::Node ratelimiter_node = root["limiter"];
    trpc::naming::RateLimiterConfig ratelimiter_config =
        ratelimiter_node["polarismesh"].as<trpc::naming::RateLimiterConfig>();
    YAML::Node selector_node = root["selector"];
    trpc::naming::SelectorConfig selector_config = selector_node["polarismesh"].as<trpc::naming::SelectorConfig>();

    selector_node["polarismesh"]["global"]["serverConnector"]["protocol"] = server_connector_plugin_name_;
    // Set the local cache location, otherwise the data of each single test will affect
    selector_node["polarismesh"]["consumer"]["localCache"]["persistDir"] = persist_dir_;
    std::stringstream strstream;
    strstream << selector_node["polarismesh"];
    std::string orig_selector_config = strstream.str();

    // Turn on Limiter related configuration
    YAML::Node node_ratelimiter;
    node_ratelimiter["rateLimiter"]["mode"] = ratelimiter_config.mode;
    strstream.str("");
    strstream << std::endl << node_ratelimiter;
    std::string orig_ratelimiter_config = strstream.str();
    orig_selector_config += orig_ratelimiter_config;

    trpc::TrpcPlugin::GetInstance()->RegisterPlugins();
    trpc::naming::PolarisMeshNamingConfig naming_config;
    naming_config.name = "polarismesh";
    naming_config.ratelimiter_config = ratelimiter_config;
    naming_config.orig_selector_config = orig_selector_config;
    naming_config.Display();

    trpc::RefPtr<trpc::PolarisMeshLimiter> p = MakeRefCounted<trpc::PolarisMeshLimiter>();
    trpc::LimiterFactory::GetInstance()->Register(p);
    limiter_ = static_pointer_cast<PolarisMeshLimiter>(trpc::LimiterFactory::GetInstance()->Get("polarismesh"));
    EXPECT_EQ(p.get(), limiter_.get());
    limiter_->SetPluginConfig(naming_config);

    // Call the Limiter plug -in function before initialization and return failure
    trpc::LimitInfo info;
    ASSERT_EQ(trpc::LimitRetCode::kLimitError, limiter_->ShouldLimit(&info));
    ASSERT_EQ(0, limiter_->Init());
    // Initialize again, return to success directly
    ASSERT_EQ(0, limiter_->Init());
    ASSERT_TRUE("" != p->Version());

    service_key_.name_ = "test.service";
    service_key_.namespace_ = kPolarisNamespaceTest;
  }

  void InitServiceNormalData() {
    polaris::FakeServer::LimitResponse(limit_response_, service_key_);
    v1::RateLimit* rate_limit = limit_response_.mutable_ratelimit();
    rate_limit->mutable_revision()->set_value("revision");
    int rules_count = 1;
    for (int i = 1; i <= rules_count; i++) {
      ::v1::Rule* rule = rate_limit->add_rules();
      rule->mutable_id()->set_value(std::to_string(i + 0));
      rule->mutable_service()->set_value(service_key_.name_);
      rule->mutable_namespace_()->set_value(service_key_.namespace_);
      rule->mutable_priority()->set_value(i - 1);
      rule->set_resource(v1::Rule::Resource::Rule_Resource_QPS);
      // Using the LOCAL mode here, there is no need to request the quota server
      rule->set_type(v1::Rule::Type::Rule_Type_LOCAL);

      v1::MatchString match_string;
      match_string.set_type(v1::MatchString::REGEX);
      match_string.mutable_value()->set_value("h*");
      (*rule->mutable_labels())["method"] = match_string;
      // (*rule->mutable_subset())["set_key"] = match_string;

      v1::Amount* amount = rule->add_amounts();
      amount->mutable_maxamount()->set_value(1);
      amount->mutable_validduration()->set_seconds(1);

      rule->mutable_action()->set_value("reject");
      rule->mutable_disable()->set_value(false);

      rule->mutable_report()->mutable_interval()->set_nanos(100000000);  // 100ms
      rule->mutable_report()->mutable_amountpercent()->set_value(5);

      // other ...
      rule->mutable_service_token()->set_value("token");

      v1::ClimbConfig* climb_config = rule->mutable_adjuster()->mutable_climb();
      climb_config->mutable_enable()->set_value(true);
      v1::ClimbConfig::MetricConfig* metric_config = climb_config->mutable_metric();
      // Window: length is 5S, accuracy is 10, the interval is 1S, each sliding window 500ms
      metric_config->mutable_window()->set_seconds(5);
      metric_config->mutable_precision()->set_value(10);
      metric_config->mutable_reportinterval()->set_seconds(1);

      v1::ClimbConfig::TriggerPolicy* policy = climb_config->mutable_policy();
      v1::ClimbConfig::TriggerPolicy::ErrorRate* error_rate = policy->mutable_errorrate();
      // Error rate: at least 10 requests to calculate the error rate, the error rate exceeds 40%trigger down
      error_rate->mutable_requestvolumethreshold()->set_value(10);
      v1::ClimbConfig::TriggerPolicy::SlowRate* slow_rate = policy->mutable_slowrate();
      // Slow call: Even if you call more than 1S, the slow call exceeds 20%to trigger down
      slow_rate->mutable_maxrt()->set_seconds(1);

      v1::ClimbConfig::ClimbThrottling* throttling = climb_config->mutable_throttling();
      // Cold water level: down 75%down, up to 65%; cold water level: down 95%, up to 80%, up to 80%,
      // Trigger the upstream regulating ratio of 2%, the judgment cycle is 2S, the trigger is raised twice in a row,
      // and the trigger will be lowered once in a row.
      throttling->mutable_judgeduration()->set_seconds(2);  // 2S judgment once
      throttling->mutable_tunedownperiod()->set_value(1);   // Trigger 1 time and down
      throttling->mutable_limitthresholdtotuneup()->set_value(
          2);  // 2%of the level above the cold water level upstream increase
    }
  }

 public:
  void MockFireEventHandler(const polaris::ServiceKey& service_key, polaris::ServiceDataType data_type,
                            uint64_t sync_interval, const std::string& disk_revision,
                            polaris::ServiceEventHandler* handler) {
    polaris::ServiceData* service_data = nullptr;
    if (data_type == polaris::kServiceDataRateLimit) {
      service_data = polaris::ServiceData::CreateFromPb(&limit_response_, polaris::kDataIsSyncing);
    } else {
      EXPECT_TRUE(false) << "unexpected data type:" << data_type;
    }
    // Create a separate thread to post data update, otherwise it will be locked
    polaris::EventHandlerData* event_data = new polaris::EventHandlerData();
    event_data->service_key_ = service_key;
    event_data->data_type_ = data_type;
    event_data->service_data_ = service_data;
    event_data->handler_ = handler;
    pthread_t tid;
    pthread_create(&tid, NULL, AsyncEventUpdate, event_data);
    handler_list_.push_back(handler);
    event_thread_list_.push_back(tid);
  }

 protected:
  trpc::PolarisMeshLimiterPtr limiter_;
  v1::DiscoverResponse limit_response_;
  polaris::ServiceKey service_key_;
  std::string persist_dir_;
  std::vector<pthread_t> event_thread_list_;
};

TEST_F(PolarisLimitTest, ShouldLimit) {
  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              RegisterEventHandler(::testing::Eq(service_key_), ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillRepeatedly(::testing::DoAll(::testing::Invoke(this, &PolarisLimitTest::MockFireEventHandler),
                                       ::testing::Return(polaris::kReturnOk)));

  trpc::LimitInfo info;
  info.name = service_key_.name_;
  info.name_space = service_key_.namespace_;
  info.labels.insert(std::make_pair("method", "hello"));

  // The first request quota within 1 second, pass
  trpc::LimitRetCode ret_code = limiter_->ShouldLimit(&info);
  ASSERT_EQ(trpc::LimitRetCode::kLimitOK, ret_code);
  // Report
  trpc::LimitResult result;
  result.limit_ret_code = ret_code;
  result.framework_result = 0;
  result.interface_result = 0;
  result.cost_time = 10;
  result.name = service_key_.name_;
  result.name_space = service_key_.namespace_;
  result.labels.insert(std::make_pair("method", "hello"));
  int ret = limiter_->FinishLimit(&result);
  ASSERT_EQ(0, ret);

  // The second request quota within 1 second, was rejected
  ret_code = limiter_->ShouldLimit(&info);
  ASSERT_EQ(ret_code, trpc::LimitRetCode::kLimitReject);
  // Report
  result.limit_ret_code = ret_code;
  ret = limiter_->FinishLimit(&result);
  ASSERT_EQ(0, ret);

  // The service name is empty, causing the internal parameter verification error of the SDK current limit plugin, and
  // the results of the report failed
  info.name = "";
  ret_code = limiter_->ShouldLimit(&info);
  ASSERT_EQ(trpc::LimitRetCode::kLimitError, ret_code);
  result.limit_ret_code = ret_code;
  ret = limiter_->FinishLimit(&result);
  ASSERT_EQ(0, ret);
}

}  // namespace trpc
