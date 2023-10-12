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

#include "trpc/naming/polarismesh/polarismesh_selector_filter.h"

#include <pthread.h>
#include <stdint.h>

#include <memory>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "polaris/model/constants.h"
#include "polaris/plugin/service_router/set_division_router.h"
#include "polaris/utils/string_utils.h"
#include "yaml-cpp/yaml.h"

#include "trpc/filter/filter.h"
#include "trpc/filter/filter_manager.h"
#include "trpc/naming/common/common_defs.h"
#include "trpc/naming/polarismesh/mock_polarismesh_api_test.h"
#include "trpc/naming/polarismesh/polarismesh_selector.h"
#include "trpc/naming/registry_factory.h"
#include "trpc/naming/selector.h"
#include "trpc/naming/selector_factory.h"
namespace trpc {

class MockProtocol : public Protocol {
 public:
  MOCK_CONST_METHOD1(GetRequestId, bool(uint32_t&));

  MOCK_METHOD1(SetRequestId, bool(uint32_t));

  /// @brief Decodes or encodes a protocol message (zero copy).
  virtual bool ZeroCopyDecode(NoncontiguousBuffer& buff) { return true; };
  virtual bool ZeroCopyEncode(NoncontiguousBuffer& buff) { return true; };
};

class PolarisMeshSelectorFilterTest : public polaris::MockServerConnectorTest {
 protected:
  virtual void SetUp() {
    MockServerConnectorTest::SetUp();
    ASSERT_TRUE(polaris::TestUtils::CreateTempDir(persist_dir_));
    InitPolarisMeshSelector();
    InitServiceNormalData();
    selector_filter_ = trpc::FilterManager::GetInstance()->GetMessageClientFilter("polarismesh");
    ASSERT_TRUE(selector_filter_ != nullptr);
    EXPECT_EQ(selector_filter_->Name(), "polarismesh");
  }

  virtual void TearDown() {
    trpc::SelectorFactory::GetInstance()->Get("polarismesh")->Destroy();
    EXPECT_TRUE(polaris::TestUtils::RemoveDir(persist_dir_));
    for (std::size_t i = 0; i < event_thread_list_.size(); ++i) {
      pthread_join(event_thread_list_[i], NULL);
    }
    MockServerConnectorTest::TearDown();
  }

  void InitPolarisMeshSelector() {
    PolarisNamingTestConfigSwitch default_Switch;
    YAML::Node root = YAML::Load(trpc::buildPolarisMeshNamingConfig(default_Switch));
    YAML::Node selector_node = root["selector"];
    trpc::naming::SelectorConfig selector_config = selector_node["polarismesh"].as<trpc::naming::SelectorConfig>();

    selector_node["polarismesh"]["global"]["serverConnector"]["protocol"] = server_connector_plugin_name_;
    // Set local regional information that is the main service, and visit the nearest visit
    selector_node["polarismesh"]["global"]["api"]["location"]["region"] = "华南";
    selector_node["polarismesh"]["global"]["api"]["location"]["zone"] = "深圳";
    selector_node["polarismesh"]["global"]["api"]["location"]["campus"] = "深圳-蛇口";
    // Set the local cache location, otherwise the data of each single test will affect
    selector_node["polarismesh"]["consumer"]["localCache"]["persistDir"] = persist_dir_;
    std::stringstream strstream;
    strstream << selector_node["polarismesh"];
    std::string orig_selector_config = strstream.str();

    trpc::naming::PolarisMeshNamingConfig naming_config;
    naming_config.name = "polarismesh";
    naming_config.selector_config = selector_config;
    naming_config.orig_selector_config = orig_selector_config;
    naming_config.Display();

    trpc::RefPtr<trpc::PolarisMeshSelector> p = MakeRefCounted<trpc::PolarisMeshSelector>();
    trpc::SelectorFactory::GetInstance()->Register(p);
    selector_ = static_pointer_cast<PolarisMeshSelector>(trpc::SelectorFactory::GetInstance()->Get("polarismesh"));
    EXPECT_EQ(p.get(), selector_.get());

    selector_->SetPluginConfig(naming_config);

    ASSERT_EQ(0, selector_->Init());
    EXPECT_TRUE("" != p->Version());

    MessageClientFilterPtr polarismesh_selector_filter(new PolarisMeshSelectorFilter());
    polarismesh_selector_filter->Init();
    FilterManager::GetInstance()->AddMessageClientFilter(polarismesh_selector_filter);

    service_key_.name_ = "test.service";
    service_key_.namespace_ = kPolarisNamespaceTest;

    v1::CircuitBreaker* cb = circuit_breaker_pb_response_.mutable_circuitbreaker();
    cb->mutable_name()->set_value("xxx");
    cb->mutable_namespace_()->set_value("xxx");
  }

  void InitServiceNormalData() {
    polaris::FakeServer::InstancesResponse(instances_response_, service_key_);
    v1::Service* service = instances_response_.mutable_service();
    // The same city that is transferred to be adjusted is nearby
    (*service->mutable_metadata())["internal-enable-nearby"] = "true";

    for (int i = 1; i <= 4; i++) {
      ::v1::Instance* instance = instances_response_.mutable_instances()->Add();
      instance->mutable_namespace_()->set_value(service_key_.namespace_);
      instance->mutable_service()->set_value(service_key_.name_);
      instance->mutable_id()->set_value("instance_" + std::to_string(i));
      instance->mutable_host()->set_value("host" + std::to_string(i));
      instance->mutable_port()->set_value(8080 + i);
      instance->mutable_healthy()->set_value(true);
      instance->mutable_weight()->set_value(100);
      instance->mutable_location()->mutable_region()->set_value("华南");
      instance->mutable_location()->mutable_zone()->set_value("深圳");
      instance->mutable_location()->mutable_campus()->set_value("深圳-蛇口");
    }
    polaris::FakeServer::RoutingResponse(routing_response_, service_key_);
  }

 public:
  void MockFireEventHandler(const polaris::ServiceKey& service_key, polaris::ServiceDataType data_type,
                            uint64_t sync_interval, const std::string& disk_revision,
                            polaris::ServiceEventHandler* handler) {
    polaris::ServiceData* service_data;
    if (data_type == polaris::kServiceDataInstances) {
      service_data = polaris::ServiceData::CreateFromPb(&instances_response_, polaris::kDataIsSyncing);
    } else if (data_type == polaris::kServiceDataRouteRule) {
      service_data = polaris::ServiceData::CreateFromPb(&routing_response_, polaris::kDataIsSyncing);
    } else if (data_type == polaris::kCircuitBreakerConfig) {
      service_data = polaris::ServiceData::CreateFromPb(&circuit_breaker_pb_response_, polaris::kDataIsSyncing);
    } else {
      service_data = polaris::ServiceData::CreateFromPb(&routing_response_, polaris::kDataIsSyncing);
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
  trpc::PolarisMeshSelectorPtr selector_;
  MessageClientFilterPtr selector_filter_;
  v1::DiscoverResponse instances_response_;
  v1::DiscoverResponse routing_response_;
  v1::DiscoverResponse circuit_breaker_pb_response_;
  polaris::ServiceKey service_key_;
  std::string persist_dir_;
  std::vector<pthread_t> event_thread_list_;
};

TEST_F(PolarisMeshSelectorFilterTest, Type) {
  auto filter_type = selector_filter_->Type();
  EXPECT_EQ(filter_type, FilterType::kSelector);
}

TEST_F(PolarisMeshSelectorFilterTest, GetFilterPoint) {
  auto filter_points = selector_filter_->GetFilterPoint();
  ASSERT_EQ(filter_points.size(), 2);
  ASSERT_EQ(filter_points[0], FilterPoint::CLIENT_PRE_RPC_INVOKE);
  ASSERT_EQ(filter_points[1], FilterPoint::CLIENT_POST_RPC_INVOKE);
}

TEST_F(PolarisMeshSelectorFilterTest, operator_select_one) {
  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              RegisterEventHandler(::testing::Eq(service_key_), ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(::testing::Exactly(2))
      .WillRepeatedly(::testing::DoAll(::testing::Invoke(this, &PolarisMeshSelectorFilterTest::MockFireEventHandler),
                                       ::testing::Return(polaris::kReturnOk)));

  // You must initialize the request before you can be selected
  ProtocolPtr request = std::make_shared<MockProtocol>();
  auto context = trpc::MakeRefCounted<ClientContext>();
  context->SetRequest(request);
  ServiceProxyOption option;
  option.name = "test";
  option.target = service_key_.name_;
  option.name_space = service_key_.namespace_;
  option.selector_name = "polarismesh";
  context->SetServiceProxyOption(&option);
  // select test
  FilterStatus status;
  selector_filter_->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_TRUE(!context->GetIp().empty());

  // report test
  selector_filter_->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  // ASSERT_TRUE(!context->GetInstanceId().empty());
}

TEST_F(PolarisMeshSelectorFilterTest, operator_select_multi) {
  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              RegisterEventHandler(::testing::Eq(service_key_), ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(::testing::Exactly(2))
      .WillRepeatedly(::testing::DoAll(::testing::Invoke(this, &PolarisMeshSelectorFilterTest::MockFireEventHandler),
                                       ::testing::Return(polaris::kReturnOk)));
  // You must initialize the request before you can be selected
  ProtocolPtr request = std::make_shared<MockProtocol>();
  auto context = trpc::MakeRefCounted<ClientContext>();
  context->SetRequest(request);
  ServiceProxyOption option;
  option.name = "test";
  option.target = service_key_.name_;
  option.name_space = service_key_.namespace_;
  option.selector_name = "polarismesh";
  context->SetServiceProxyOption(&option);
  context->SetBackupRequestDelay(10);  // Use Backup Request, select two nodes
  ASSERT_EQ(context->GetBackupRequestRetryInfo()->backup_addrs.size(), 0);
  //::trpc::naming::polarismesh::SetSelectorExtendInfo(context, std::make_pair("namespace", "Test"));
  // select test
  FilterStatus status;
  selector_filter_->operator()(status, FilterPoint::CLIENT_PRE_RPC_INVOKE, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_TRUE(!context->GetIp().empty());
  ASSERT_EQ(context->GetBackupRequestRetryInfo()->backup_addrs.size(), 2);
  ASSERT_EQ(context->GetPort(), context->GetBackupRequestRetryInfo()->backup_addrs[0].addr.port);
  // Set the second node call success
  context->GetBackupRequestRetryInfo()->succ_rsp_node_index = 1;

  // report test
  selector_filter_->operator()(status, FilterPoint::CLIENT_POST_RPC_INVOKE, context);
  ASSERT_EQ(status, FilterStatus::CONTINUE);
  ASSERT_EQ(context->GetPort(), context->GetBackupRequestRetryInfo()
                                    ->backup_addrs[context->GetBackupRequestRetryInfo()->succ_rsp_node_index]
                                    .addr.port);
}

}  // namespace trpc
