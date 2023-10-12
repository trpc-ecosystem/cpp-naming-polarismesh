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

#include "trpc/naming/polarismesh/polarismesh_selector.h"

#include <pthread.h>
#include <stdint.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "polaris/model/constants.h"
#include "polaris/plugin/service_router/set_division_router.h"
#include "polaris/utils/string_utils.h"
#include "polaris/utils/time_clock.h"
#include "yaml-cpp/yaml.h"

#include "trpc/naming/polarismesh/mock_polarismesh_api_test.h"
#include "trpc/naming/registry_factory.h"
#include "trpc/naming/selector.h"
#include "trpc/naming/selector_factory.h"
#include "trpc/util/ref_ptr.h"

namespace trpc {

class MockProtocol : public Protocol {
 public:
  MOCK_CONST_METHOD1(GetRequestId, bool(uint32_t&));

  MOCK_METHOD1(SetRequestId, bool(uint32_t));

  /// @brief Decodes or encodes a protocol message (zero copy).
  virtual bool ZeroCopyDecode(NoncontiguousBuffer& buff) { return true; };
  virtual bool ZeroCopyEncode(NoncontiguousBuffer& buff) { return true; };
};

class PolarisSelectTest : public polaris::MockServerConnectorTest {
 protected:
  virtual void SetUp() {
    MockServerConnectorTest::SetUp();
    ASSERT_TRUE(polaris::TestUtils::CreateTempDir(persist_dir_));
    InitPolarisMeshSelector();
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
    // Turn on metadata route plug -in
    default_Switch.need_dstMetaRouter = true;
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
    EXPECT_TRUE("" != selector_->Version());

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

    for (int i = 1; i <= 6; i++) {
      ::v1::Instance* instance = instances_response_.mutable_instances()->Add();
      instance->mutable_namespace_()->set_value(service_key_.namespace_);
      instance->mutable_service()->set_value(service_key_.name_);
      instance->mutable_id()->set_value("instance_" + std::to_string(i));
      instance->mutable_host()->set_value("host" + std::to_string(i));
      instance->mutable_port()->set_value(8080 + i);

      // Node 1 and Node 2: Healthy Shenzhen Node
      if (i == 1 || i == 2) {
        instance->mutable_healthy()->set_value(true);
        instance->mutable_weight()->set_value(100);
        instance->mutable_location()->mutable_region()->set_value("华南");
        instance->mutable_location()->mutable_zone()->set_value("深圳");
        instance->mutable_location()->mutable_campus()->set_value("深圳-蛇口");
      }

      // Node 3: Healthy Shanghai node
      if (i == 3) {
        instance->mutable_healthy()->set_value(true);
        instance->mutable_weight()->set_value(100);
        instance->mutable_location()->mutable_region()->set_value("华东");
        instance->mutable_location()->mutable_zone()->set_value("上海");
        instance->mutable_location()->mutable_campus()->set_value("上海-浦东");
      }

      // Node 4: Unhealthy Shenzhen node
      if (i == 4) {
        instance->mutable_healthy()->set_value(false);
        instance->mutable_weight()->set_value(100);
        instance->mutable_location()->mutable_region()->set_value("华南");
        instance->mutable_location()->mutable_zone()->set_value("深圳");
        instance->mutable_location()->mutable_campus()->set_value("深圳-蛇口");
      }

      // Node 5: Healthy Shenzhen node, but the weight is 0
      if (i == 5) {
        instance->mutable_healthy()->set_value(true);
        instance->mutable_weight()->set_value(0);
        instance->mutable_location()->mutable_region()->set_value("华南");
        instance->mutable_location()->mutable_zone()->set_value("深圳");
        instance->mutable_location()->mutable_campus()->set_value("深圳-蛇口");
      }

      // Node 6: Isolate Shenzhen Node
      if (i == 6) {
        instance->mutable_healthy()->set_value(true);
        instance->mutable_weight()->set_value(100);
        instance->mutable_location()->mutable_region()->set_value("华东");
        instance->mutable_location()->mutable_zone()->set_value("上海");
        instance->mutable_location()->mutable_campus()->set_value("上海-浦东");
        instance->mutable_isolate()->set_value(true);
      }
    }
    polaris::FakeServer::RoutingResponse(routing_response_, service_key_);
  }

  void InitServiceSetData() {
    polaris::FakeServer::InstancesResponse(instances_response_, service_key_);
    for (int i = 1; i <= 2; i++) {
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
      // Open set
      (*instance->mutable_metadata())[polaris::SetDivisionServiceRouter::enable_set_key] = "Y";

      // Node 1: SET name is app.sz.1
      if (i == 1) {
        (*instance->mutable_metadata())[polaris::constants::kRouterRequestSetNameKey] = "app.sz.1";
      }

      // Node 1: SET name is app.sz.2
      if (i == 2) {
        (*instance->mutable_metadata())[polaris::constants::kRouterRequestSetNameKey] = "app.sz.2";
      }
    }
    polaris::FakeServer::RoutingResponse(routing_response_, service_key_);
  }

  void InitServiceCanaryData() {
    polaris::FakeServer::InstancesResponse(instances_response_, service_key_);
    v1::Service* service = instances_response_.mutable_service();
    // Turn on the Jin Silk route that is transferred
    (*service->mutable_metadata())["internal-canary"] = "true";

    for (int i = 1; i <= 2; i++) {
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

      // Node 1: Golden Silk Node
      if (i == 1) {
        (*instance->mutable_metadata())["canary"] = "1";
      }
    }
    polaris::FakeServer::RoutingResponse(routing_response_, service_key_);
  }

  void InitServiceDstMetaData() {
    polaris::FakeServer::InstancesResponse(instances_response_, service_key_);
    for (int i = 1; i <= 2; i++) {
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

      // Node 1: Bringing metadata tag
      if (i == 1) {
        (*instance->mutable_metadata())["label"] = "test";
      }
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
  v1::DiscoverResponse instances_response_;
  v1::DiscoverResponse routing_response_;
  v1::DiscoverResponse circuit_breaker_pb_response_;
  polaris::ServiceKey service_key_;
  std::string persist_dir_;
  std::vector<pthread_t> event_thread_list_;
};

TEST_F(PolarisSelectTest, SelectNormal) {
  InitServiceNormalData();

  // You must initialize the request before you can be selected
  ProtocolPtr request = std::make_shared<MockProtocol>();

  auto select_context = trpc::MakeRefCounted<trpc::ClientContext>();
  select_context->SetRequest(request);
  trpc::RefPtr<trpc::PolarisMeshSelector> p = static_pointer_cast<trpc::PolarisMeshSelector>(selector_);
  trpc::naming::polarismesh::SetSelectorExtendInfo(select_context, std::make_pair("namespace", service_key_.namespace_));
  trpc::SelectorInfo selectInfo;
  selectInfo.name = service_key_.name_;
  selectInfo.context = select_context;
  selectInfo.load_balance_name = polaris::kLoadBalanceTypeDefaultConfig;
  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              RegisterEventHandler(::testing::Eq(service_key_), ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(::testing::Exactly(2))
      .WillRepeatedly(::testing::DoAll(::testing::Invoke(this, &PolarisSelectTest::MockFireEventHandler),
                                       ::testing::Return(polaris::kReturnOk)));

  trpc::TrpcEndpointInfo endpoint;
  int ret = selector_->Select(&selectInfo, &endpoint);
  ASSERT_EQ(0, ret);
  ASSERT_FALSE(endpoint.meta["instance_id"].empty());

  trpc::InvokeResult result;
  result.name = service_key_.name_;
  result.framework_result = trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS;
  result.interface_result = 0;
  result.cost_time = 100;
  auto result_context = trpc::MakeRefCounted<trpc::ClientContext>();
  result_context->SetRequest(request);
  trpc::naming::polarismesh::SetSelectorExtendInfo(result_context, std::make_pair("namespace", service_key_.namespace_));
  // Using IPPORT to report, you need to have cache data in SDK first. SDK will reflect the opePort into Instanceid
  result_context->SetAddr(endpoint.host, endpoint.port);
  result.context = result_context;
  ret = selector_->ReportInvokeResult(&result);
  ASSERT_EQ(0, ret);

  selector_->AsyncSelect(&selectInfo)
      .Then([this](trpc::Future<trpc::TrpcEndpointInfo>&& select_fut) {
        EXPECT_TRUE(select_fut.IsReady());
        auto endpoint = select_fut.GetValue0();
        EXPECT_FALSE(endpoint.meta["instance_id"].empty());
        return trpc::MakeReadyFuture<>();
      })
      .Wait();

  // Test other versions of the load balancing strategy
  // Because the nearby visits are turned on, the node collection of the load balancing strategy is only 2 nodes in
  // Shenzhen

  // Ring Hash algorithm, SDK guarantees that the nodes on the Hash ring are evenly distributed, and the polarismesh
  // plug -in can be verified by the actual operation results of the front and rear versions
  selectInfo.load_balance_name = polaris::kLoadBalanceTypeRingHash;
  selectInfo.context->SetHashKey("abc");
  ASSERT_EQ(0, selector_->Select(&selectInfo, &endpoint));
  // Take the Hash algorithm, use Hash Key to take the total number of nodes
  selectInfo.load_balance_name = polaris::kLoadBalanceTypeSimpleHash;
  selectInfo.context->SetHashKey("0");
  ASSERT_EQ(0, selector_->Select(&selectInfo, &endpoint));
  ASSERT_EQ(endpoint.host, "host1");
  selectInfo.context->SetHashKey("1");
  ASSERT_EQ(0, selector_->Select(&selectInfo, &endpoint));
  ASSERT_EQ(endpoint.host, "host2");
  selectInfo.context->SetHashKey("2");
  ASSERT_EQ(0, selector_->Select(&selectInfo, &endpoint));
  ASSERT_EQ(endpoint.host, "host1");

  // The consistency HASH algorithm used in L5, the SDK guarantees that the output is the same as the L5 client. The
  // polarismesh plug -in can be verified by the actual operation results of the front and rear versions
  selectInfo.load_balance_name = polaris::kLoadBalanceTypeL5CstHash;
  selectInfo.context->SetHashKey("abc");
  ASSERT_EQ(0, selector_->Select(&selectInfo, &endpoint));
}

TEST_F(PolarisSelectTest, SelectAllNormal) {
  InitServiceNormalData();

  // You must initialize the request before you can be selected
  ProtocolPtr request = std::make_shared<MockProtocol>();

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  context->SetRequest(request);
  trpc::RefPtr<trpc::PolarisMeshSelector> p = static_pointer_cast<trpc::PolarisMeshSelector>(selector_);
  trpc::naming::polarismesh::SetSelectorExtendInfo(context, std::make_pair("namespace", service_key_.namespace_));
  trpc::SelectorInfo selectInfo;
  selectInfo.name = service_key_.name_;
  selectInfo.context = context;

  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              RegisterEventHandler(::testing::Eq(service_key_), ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(1)  // No need to call routing information
      .WillRepeatedly(::testing::DoAll(::testing::Invoke(this, &PolarisSelectTest::MockFireEventHandler),
                                       ::testing::Return(polaris::kReturnOk)));

  // Strategy: Return all nodes, including isolation and weight 0
  selectInfo.policy = trpc::SelectorPolicy::ALL;
  std::vector<trpc::TrpcEndpointInfo> endpoints;
  int ret = selector_->SelectBatch(&selectInfo, &endpoints);
  EXPECT_EQ(0, ret);
  // Node 5 with a weight of 0, and the isolation node 6 without returning
  EXPECT_EQ(endpoints.size(), 4);

  selector_->AsyncSelectBatch(&selectInfo)
      .Then([this, context, &selectInfo](trpc::Future<std::vector<TrpcEndpointInfo>>&& select_fut) {
        EXPECT_TRUE(select_fut.IsReady());
        auto endpoints = select_fut.GetValue0();
        // Node 5 with a weight of 0, and the isolation node 6 without returning
        EXPECT_EQ(endpoints.size(), 4);
        return trpc::MakeReadyFuture<>();
      })
      .Wait();
}

TEST_F(PolarisSelectTest, SelectBatchNormal) {
  InitServiceNormalData();

  // You must initialize the request before you can be selected
  ProtocolPtr request = std::make_shared<MockProtocol>();

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  context->SetRequest(request);
  trpc::RefPtr<trpc::PolarisMeshSelector> p = static_pointer_cast<trpc::PolarisMeshSelector>(selector_);
  trpc::naming::polarismesh::SetSelectorExtendInfo(context, std::make_pair("namespace", service_key_.namespace_));
  trpc::SelectorInfo selectInfo;
  selectInfo.name = service_key_.name_;
  selectInfo.context = context;

  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              RegisterEventHandler(::testing::Eq(service_key_), ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(::testing::Exactly(2))
      .WillRepeatedly(::testing::DoAll(::testing::Invoke(this, &PolarisSelectTest::MockFireEventHandler),
                                       ::testing::Return(polaris::kReturnOk)));

  // Strategy: Back to all normal nodes near the same city
  selectInfo.policy = trpc::SelectorPolicy::IDC;
  std::vector<trpc::TrpcEndpointInfo> healthy_endpoints;
  int ret = selector_->SelectBatch(&selectInfo, &healthy_endpoints);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(2, healthy_endpoints.size());  // Node 1 and Node 2

  // The strategy is still the same city, but including unhealthy nodes
  std::vector<trpc::TrpcEndpointInfo> include_unhealthy_endpoints;

  trpc::naming::polarismesh::SetSelectorExtendInfo(context, std::make_pair("include_unhealthy", "true"));
  ret = selector_->SelectBatch(&selectInfo, &include_unhealthy_endpoints);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(3, include_unhealthy_endpoints.size());  // Node 1, node 2 and node 4
}

TEST_F(PolarisSelectTest, SelectSet) {
  InitServiceSetData();

  // You must initialize the request before you can be selected
  ProtocolPtr request = std::make_shared<MockProtocol>();

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  context->SetRequest(request);
  trpc::RefPtr<trpc::PolarisMeshSelector> p = static_pointer_cast<trpc::PolarisMeshSelector>(selector_);
  // Use SET route
  trpc::naming::polarismesh::SetSelectorExtendInfo(context, std::make_pair("namespace", service_key_.namespace_), std::make_pair("callee_set_name", "app.sz.1"), std::make_pair("enable_set_force", "true"));
  trpc::SelectorInfo selectInfo;
  selectInfo.name = service_key_.name_;
  selectInfo.context = context;

  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              RegisterEventHandler(::testing::Eq(service_key_), ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(::testing::Exactly(2))
      .WillRepeatedly(::testing::DoAll(::testing::Invoke(this, &PolarisSelectTest::MockFireEventHandler),
                                       ::testing::Return(polaris::kReturnOk)));

  trpc::TrpcEndpointInfo endpoint;
  int ret = selector_->Select(&selectInfo, &endpoint);
  ASSERT_EQ(0, ret);
  ASSERT_EQ("instance_1", endpoint.meta["instance_id"]);

  selectInfo.policy = trpc::SelectorPolicy::SET;
  std::vector<trpc::TrpcEndpointInfo> endpoints;
  ret = selector_->SelectBatch(&selectInfo, &endpoints);
  ASSERT_EQ(0, ret);
  ASSERT_EQ(1, endpoints.size());
  ASSERT_EQ("host1", endpoints[0].host);
}

TEST_F(PolarisSelectTest, SelectCanary) {
  InitServiceCanaryData();
  // You must initialize the request before you can be selected
  ProtocolPtr request = std::make_shared<MockProtocol>();

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  context->SetRequest(request);
  trpc::RefPtr<trpc::PolarisMeshSelector> p = static_pointer_cast<trpc::PolarisMeshSelector>(selector_);
  trpc::naming::polarismesh::SetSelectorExtendInfo(context, std::make_pair("namespace", service_key_.namespace_));
  trpc::SelectorInfo selectInfo;
  selectInfo.name = service_key_.name_;
  selectInfo.context = context;

  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              RegisterEventHandler(::testing::Eq(service_key_), ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(::testing::Exactly(2))
      .WillRepeatedly(::testing::DoAll(::testing::Invoke(this, &PolarisSelectTest::MockFireEventHandler),
                                       ::testing::Return(polaris::kReturnOk)));

  // There is no Canary settings, returning non -golden bird nodes
  trpc::TrpcEndpointInfo Uncanary_endpoint;
  int ret = selector_->Select(&selectInfo, &Uncanary_endpoint);
  ASSERT_EQ(0, ret);
  ASSERT_EQ("instance_2", Uncanary_endpoint.meta["instance_id"]);

  // The main tone is Canary settings, returned to the node
  std::string canary_value = "1";
  trpc::naming::polarismesh::SetSelectorExtendInfo(context, std::make_pair("canary_label", canary_value));
  trpc::TrpcEndpointInfo canary_endpoint;
  ret = selector_->Select(&selectInfo, &canary_endpoint);
  ASSERT_EQ(0, ret);
  // Canary has a problem skipping
  // ASSERT_EQ("instance_1", canary_endpoint.meta["instance_id"]);
}

TEST_F(PolarisSelectTest, SelectDstMeta) {
  InitServiceDstMetaData();

  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              RegisterEventHandler(::testing::Eq(service_key_), ::testing::_, ::testing::_, ::testing::_, ::testing::_))
      .Times(::testing::Exactly(2))
      .WillRepeatedly(::testing::DoAll(::testing::Invoke(this, &PolarisSelectTest::MockFireEventHandler),
                                       ::testing::Return(polaris::kReturnOk)));
  // You must initialize the request before you can be selected
  ProtocolPtr request = std::make_shared<MockProtocol>();

  // The main tone is the metadata label setting, and the corresponding node of the corresponding data label is returned
  for (int i = 0; i < 50; i++) {
    auto context = trpc::MakeRefCounted<trpc::ClientContext>();
    context->SetRequest(request);
    trpc::RefPtr<trpc::PolarisMeshSelector> p = static_pointer_cast<trpc::PolarisMeshSelector>(selector_);
    trpc::naming::polarismesh::SetSelectorExtendInfo(context, std::make_pair("namespace", service_key_.namespace_));
    std::map<std::string, std::string> meta;
    meta["label"] = "test";
    trpc::naming::polarismesh::SetFilterMetadataOfNaming(context, meta, trpc::PolarisMetadataType::kPolarisDstMetaRouteLable);
    trpc::SelectorInfo selectInfo;
    selectInfo.name = service_key_.name_;
    selectInfo.context = context;
    trpc::TrpcEndpointInfo meta_endpoint;
    int ret = selector_->Select(&selectInfo, &meta_endpoint);
    ASSERT_EQ(0, ret);
    ASSERT_EQ("instance_1", meta_endpoint.meta["instance_id"]);
  }
}

TEST_F(PolarisSelectTest, ReportInvokeResult) {
  trpc::InvokeResult result;
  result.name = service_key_.name_;
  result.framework_result = trpc::TrpcRetCode::TRPC_INVOKE_SUCCESS;
  result.interface_result = 0;
  result.cost_time = 100;
  // You must initialize the request before you can be selected
  ProtocolPtr request = std::make_shared<MockProtocol>();

  auto context = trpc::MakeRefCounted<trpc::ClientContext>();
  context->SetRequest(request);
  trpc::RefPtr<trpc::PolarisMeshSelector> p = static_pointer_cast<trpc::PolarisMeshSelector>(selector_);
  // Report using Instanceid, no need to construct cache data at this time
  // context->SetInstanceId("instance_1");
  // Reporting with LocalityawareInfo
  trpc::naming::polarismesh::SetSelectorExtendInfo(context, std::make_pair("locality_aware_info", "10"), std::make_pair("namespace", service_key_.namespace_));
  //p->SetLocalityAwareInfo(10);
  result.context = context;
  // Set White List
  std::vector<int> white_list = {TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR, TrpcRetCode::TRPC_SERVER_FULL_LINK_TIMEOUT_ERR};
  ASSERT_TRUE(selector_->SetCircuitBreakWhiteList(white_list));
  // ASSERT_EQ(0, selector_->ReportInvokeResult(&result));
}

}  // namespace trpc

class PolarisTestEnvironment : public testing::Environment {
 public:
  virtual void SetUp() {}
  virtual void TearDown() { trpc::SelectorFactory::GetInstance()->Clear(); }
};

int main(int argc, char** argv) {
  testing::AddGlobalTestEnvironment(new PolarisTestEnvironment);
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
