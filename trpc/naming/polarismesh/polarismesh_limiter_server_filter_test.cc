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

#include "trpc/naming/polarismesh/polarismesh_limiter_server_filter.h"

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

#include "trpc/codec/trpc/trpc_protocol.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/naming/polarismesh/mock_polarismesh_api_test.h"
#include "trpc/naming/polarismesh/polarismesh_limiter.h"
#include "trpc/server/rpc_service_impl.h"
#include "trpc/server/testing/service_adapter_testing.h"

namespace trpc {
namespace testing {

class PolarisMeshLimiterServerFilterTest : public polaris::MockServerConnectorTest {
 protected:
  static void SetUpTestCase() { trpc::TrpcPlugin::GetInstance()->RegisterPlugins(); }

  static void TearDownTestCase() { trpc::TrpcPlugin::GetInstance()->UnregisterPlugins(); }

  virtual void SetUp() {
    MockServerConnectorTest::SetUp();
    ASSERT_TRUE(polaris::TestUtils::CreateTempDir(persist_dir_));
    InitPolarisMeshLimiter();
    InitServiceNormalData();
    polarismesh_limiter_server_filter_ = FilterManager::GetInstance()->GetMessageServerFilter("polarismesh_limiter");
    ASSERT_TRUE(nullptr != polarismesh_limiter_server_filter_);
  }

  virtual void TearDown() {
    trpc::LimiterFactory::GetInstance()->Get("polarismesh")->Destroy();
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

    ASSERT_EQ(0, limiter_->Init());
    EXPECT_TRUE("" != p->Version());

    MessageServerFilterPtr polarismesh_limiter_server_filter(new PolarisMeshLimiterServerFilter());
    polarismesh_limiter_server_filter->Init();
    FilterManager::GetInstance()->AddMessageServerFilter(polarismesh_limiter_server_filter);

    service_key_.name_ = "test.service";
    // The current -limited interceptor of the server reads the environment name in the Global configuration
    service_key_.namespace_ = "Development";
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

      v1::Amount* amount = rule->add_amounts();
      amount->mutable_maxamount()->set_value(1);
      amount->mutable_validduration()->set_seconds(1);
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
  MessageServerFilterPtr polarismesh_limiter_server_filter_;
  v1::DiscoverResponse limit_response_;
  polaris::ServiceKey service_key_;
  std::string persist_dir_;
  std::vector<pthread_t> event_thread_list_;
};

TEST_F(PolarisMeshLimiterServerFilterTest, Name) { ASSERT_EQ("polarismesh_limiter", polarismesh_limiter_server_filter_->Name()); }

TEST_F(PolarisMeshLimiterServerFilterTest, GetFilterPointTest) {
  auto points = polarismesh_limiter_server_filter_->GetFilterPoint();
  ASSERT_EQ(2, points.size());
  ASSERT_EQ(points[0], FilterPoint::SERVER_PRE_RPC_INVOKE);
  ASSERT_EQ(points[1], FilterPoint::SERVER_POST_RPC_INVOKE);
}

TEST_F(PolarisMeshLimiterServerFilterTest, operator) {
  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              RegisterEventHandler(::testing::Eq(service_key_), ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillRepeatedly(::testing::DoAll(::testing::Invoke(this, &PolarisMeshLimiterServerFilterTest::MockFireEventHandler),
                                       ::testing::Return(polaris::kReturnOk)));

  // Initialize the environment name in the Global configuration
  auto ret = trpc::TrpcConfig::GetInstance()->Init("./trpc/naming/polarismesh/testing/polarismesh_test.yaml");
  ASSERT_EQ(0, ret);
  ASSERT_EQ(service_key_.namespace_, TrpcConfig::GetInstance()->GetGlobalConfig().env_namespace);

  auto context = MakeRefCounted<ServerContext>();
  context->SetRequestMsg(std::make_shared<TrpcRequestProtocol>());
  ServicePtr service = std::make_shared<RpcServiceImpl>();
  trpc::ServiceAdapterOption option;
  option.protocol = "trpc";
  option.service_name = service_key_.name_;
  trpc::ServiceAdapter adapter(std::move(option));
  trpc::testing::FillServiceAdapter(&adapter, service_key_.name_, service);
  context->SetService(service.get());
  context->SetFuncName("/test.service/hello");

  FilterStatus status;
  // When the current report function is closed, the report is reported directly to return
  polarismesh_limiter_server_filter_->operator()(status, FilterPoint::SERVER_POST_RPC_INVOKE, context);
  ASSERT_EQ(FilterStatus::CONTINUE, status);

  // Re -read the configuration item, turn on the restrictions on the reporting function
  polarismesh_limiter_server_filter_->Init();

  // The first request quota within 1 second, pass
  polarismesh_limiter_server_filter_->operator()(status, FilterPoint::SERVER_PRE_RPC_INVOKE, context);
  ASSERT_EQ(FilterStatus::CONTINUE, status);

  // Report
  polarismesh_limiter_server_filter_->operator()(status, FilterPoint::SERVER_POST_RPC_INVOKE, context);
  ASSERT_EQ(FilterStatus::CONTINUE, status);

  // The second request quota within 1 second, was rejected
  polarismesh_limiter_server_filter_->operator()(status, FilterPoint::SERVER_PRE_RPC_INVOKE, context);
  ASSERT_EQ(FilterStatus::REJECT, status);
}

}  // namespace testing

}  // namespace trpc
