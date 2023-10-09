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

#include "trpc/naming/polarismesh/polaris_registry.h"

#include <pthread.h>
#include <stdint.h>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "yaml-cpp/yaml.h"

#include "trpc/common/trpc_plugin.h"
#include "trpc/naming/polarismesh/mock_polaris_api_test.h"
#include "trpc/naming/registry.h"
#include "trpc/naming/registry_factory.h"

namespace trpc {

// There is no Instanceid in the configuration file
class PolarisRegistryWithoutInstanceIdTest : public polaris::MockServerConnectorTest {
 protected:
  virtual void SetUp() {
    polaris::MockServerConnectorTest::SetUp();
    InitPolarisNoInstanceID();
  }

  virtual void TearDown() {
    trpc::RegistryFactory::GetInstance()->Get("polarismesh")->Destroy();
    // Destruction again, return directly
    trpc::RegistryFactory::GetInstance()->Get("polarismesh")->Destroy();
    MockServerConnectorTest::TearDown();
    registry_ = nullptr;
  }

  void InitPolarisNoInstanceID() {
    PolarisNamingTestConfigSwitch default_Switch;
    YAML::Node root = YAML::Load(trpc::buildPolarisNamingConfig(default_Switch));
    YAML::Node registry_node = root["registry"];
    trpc::naming::RegistryConfig registry_config = registry_node["polarismesh"].as<trpc::naming::RegistryConfig>();
    // Use IP: Port to report
    registry_config.services_config[0].instance_id = "";

    YAML::Node selector_node = root["selector"];
    selector_node["polarismesh"]["global"]["serverConnector"]["protocol"] = server_connector_plugin_name_;
    std::stringstream strstream;
    strstream << selector_node["polarismesh"];
    std::string orig_selector_config = strstream.str();

    trpc::naming::PolarisNamingConfig naming_config;
    naming_config.name = "polarismesh";
    naming_config.registry_config = registry_config;
    naming_config.orig_selector_config = orig_selector_config;
    naming_config.Display();

    trpc::RefPtr<trpc::PolarisRegistry> p = MakeRefCounted<trpc::PolarisRegistry>();
    trpc::RegistryFactory::GetInstance()->Register(p);
    registry_ = static_pointer_cast<PolarisRegistry>(trpc::RegistryFactory::GetInstance()->Get("polarismesh"));
    EXPECT_EQ(p.get(), registry_.get());

    registry_->SetPluginConfig(naming_config);
    // Before initialization, the call method returns directly to fail
    RegistryInfo register_info;
    ASSERT_EQ(-1, registry_->Register(&register_info));
    ASSERT_EQ(-1, registry_->Unregister(&register_info));
    ASSERT_EQ(-1, registry_->HeartBeat(&register_info));
    registry_->AsyncHeartBeat(&register_info)
        .Then([](trpc::Future<>&& registry_fut) {
          EXPECT_TRUE(registry_fut.IsFailed());
          return trpc::MakeReadyFuture<>();
        })
        .Wait();

    ASSERT_EQ(0, registry_->Init());
    // Initialize again, return to success directly
    ASSERT_EQ(0, registry_->Init());
    EXPECT_TRUE("" != p->Version());
  }

 protected:
  trpc::PolarisRegistryPtr registry_;
};

TEST_F(PolarisRegistryWithoutInstanceIdTest, Register) {
  std::string callee_service = "test.service";
  trpc::RegistryInfo register_info;
  register_info.host = "127.0.0.1";
  register_info.port = 10001;
  register_info.meta["namespace"] = kPolarisNamespaceTest;

  std::string return_instance = "return_instance";
  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              RegisterInstance(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillOnce(::testing::DoAll(::testing::SetArgReferee<2>(return_instance),
                                 ::testing::Return(polaris::ReturnCode::kReturnOk)));
  // In the empty parameters, it fails directly
  trpc::RegistryInfo* register_info_no_exist = nullptr;
  int ret = registry_->Register(register_info_no_exist);
  EXPECT_EQ(-1, ret);

  // No information about the name in register config
  register_info.name = "test.service.no";
  ret = registry_->Register(&register_info);
  EXPECT_EQ(-1, ret);
  // There is information about the name in register config
  register_info.name = callee_service;
  ret = registry_->Register(&register_info);
  ASSERT_EQ(0, ret);
  ASSERT_EQ("return_instance", register_info.meta["instance_id"]);
}

TEST_F(PolarisRegistryWithoutInstanceIdTest, Unregister) {
  std::string callee_service = "test.service";
  trpc::RegistryInfo register_info;
  register_info.host = "127.0.0.1";
  register_info.port = 10001;
  register_info.meta["namespace"] = kPolarisNamespaceTest;

  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_, DeregisterInstance(::testing::_, ::testing::_))
      .Times(1)
      .WillRepeatedly(::testing::Return(polaris::ReturnCode::kReturnOk));

  // In the empty parameters, it fails directly
  trpc::RegistryInfo* register_info_no_exist = nullptr;
  int ret = registry_->Unregister(register_info_no_exist);
  EXPECT_EQ(-1, ret);

  // No information about the name in register config
  register_info.name = "test.service.no";
  ret = registry_->Unregister(&register_info);
  EXPECT_EQ(-1, ret);
  // There is information about the name in register config
  register_info.name = callee_service;
  ret = registry_->Unregister(&register_info);
  ASSERT_EQ(0, ret);
}

TEST_F(PolarisRegistryWithoutInstanceIdTest, HeartBeat) {
  std::string callee_service = "test.service";
  trpc::RegistryInfo register_info;
  register_info.host = "127.0.0.1";
  register_info.port = 10001;
  register_info.meta["namespace"] = kPolarisNamespaceTest;

  std::string return_instance = "return_instance";
  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_, InstanceHeartbeat(::testing::_, ::testing::_))
      .Times(1)
      .WillRepeatedly(::testing::Return(polaris::ReturnCode::kReturnOk));

  // In the empty parameters, it fails directly
  trpc::RegistryInfo* register_info_no_exist = nullptr;
  int ret = registry_->HeartBeat(register_info_no_exist);
  EXPECT_EQ(-1, ret);

  // No information about the name in register config
  register_info.name = "test.service.no";
  ret = registry_->HeartBeat(&register_info);
  EXPECT_EQ(-1, ret);
  // There is information about the name in register config
  register_info.name = callee_service;
  ret = registry_->HeartBeat(&register_info);
  ASSERT_EQ(0, ret);
}

TEST_F(PolarisRegistryWithoutInstanceIdTest, AsyncHeartBeat) {
  std::string callee_service = "test.service";
  trpc::RegistryInfo register_info;
  register_info.host = "127.0.0.1";
  register_info.port = 10001;
  register_info.meta["namespace"] = kPolarisNamespaceTest;

  std::string return_instance = "return_instance";
  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              AsyncInstanceHeartbeat(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillRepeatedly(::testing::Return(polaris::ReturnCode::kReturnOk));

  // In the empty parameters, it fails directly
  trpc::RegistryInfo* register_info_no_exist = nullptr;
  registry_->AsyncHeartBeat(register_info_no_exist)
      .Then([](trpc::Future<>&& registry_fut) {
        EXPECT_TRUE(registry_fut.IsFailed());
        return trpc::MakeReadyFuture<>();
      })
      .Wait();

  // No information about the name in register config
  register_info.name = "test.service.no";
  registry_->AsyncHeartBeat(&register_info)
      .Then([](trpc::Future<>&& registry_fut) {
        EXPECT_TRUE(registry_fut.IsFailed());
        return trpc::MakeReadyFuture<>();
      })
      .Wait();
  // There is information about the name in register config
  register_info.name = callee_service;
  registry_->AsyncHeartBeat(&register_info)
      .Then([](trpc::Future<>&& registry_fut) {
        EXPECT_TRUE(registry_fut.IsReady());
        return trpc::MakeReadyFuture<>();
      })
      .Wait();
}

// The configuration file contains Instanceid
class PolarisRegistryWithInstanceIdTest : public polaris::MockServerConnectorTest {
 protected:
  virtual void SetUp() {
    polaris::MockServerConnectorTest::SetUp();
    InitPolarisRegistryWithInstanceId();
  }

  virtual void TearDown() {
    trpc::RegistryFactory::GetInstance()->Get("polarismesh")->Destroy();
    MockServerConnectorTest::TearDown();
    registry_ = nullptr;
  }

  void InitPolarisRegistryWithInstanceId() {
    PolarisNamingTestConfigSwitch default_Switch;
    YAML::Node root = YAML::Load(trpc::buildPolarisNamingConfig(default_Switch));
    YAML::Node registry_node = root["registry"];
    trpc::naming::RegistryConfig registry_config = registry_node["polarismesh"].as<trpc::naming::RegistryConfig>();

    YAML::Node selector_node = root["selector"];
    selector_node["polarismesh"]["global"]["serverConnector"]["protocol"] = server_connector_plugin_name_;
    std::stringstream strstream;
    strstream << selector_node["polarismesh"];
    std::string orig_selector_config = strstream.str();

    trpc::naming::PolarisNamingConfig naming_config;
    naming_config.name = "polarismesh";
    naming_config.registry_config = registry_config;
    naming_config.orig_selector_config = orig_selector_config;
    naming_config.Display();

    trpc::RefPtr<trpc::PolarisRegistry> p = MakeRefCounted<trpc::PolarisRegistry>();
    trpc::RegistryFactory::GetInstance()->Register(p);
    registry_ = static_pointer_cast<PolarisRegistry>(trpc::RegistryFactory::GetInstance()->Get("polarismesh"));
    EXPECT_EQ(p.get(), registry_.get());

    registry_->SetPluginConfig(naming_config);
    ASSERT_EQ(0, registry_->Init());
    EXPECT_TRUE("" != p->Version());
  }

 protected:
  trpc::PolarisRegistryPtr registry_;
};

TEST_F(PolarisRegistryWithInstanceIdTest, Unregister) {
  std::string callee_service = "test.service";
  trpc::RegistryInfo register_info;
  register_info.meta["namespace"] = kPolarisNamespaceTest;
  // If the anti -registered Instance_id is needed, it is passed from the parameter, not obtained from the configuration
  register_info.meta["instance_id"] = "instance_id";

  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_, DeregisterInstance(::testing::_, ::testing::_))
      .Times(1)
      .WillRepeatedly(::testing::Return(polaris::ReturnCode::kReturnOk));

  register_info.name = callee_service;
  int ret = registry_->Unregister(&register_info);
  ASSERT_EQ(0, ret);
}

TEST_F(PolarisRegistryWithInstanceIdTest, HeartBeat) {
  std::string callee_service = "test.service";
  trpc::RegistryInfo register_info;
  register_info.meta["namespace"] = kPolarisNamespaceTest;

  std::string return_instance = "return_instance";
  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_, InstanceHeartbeat(::testing::_, ::testing::_))
      .Times(1)
      .WillRepeatedly(::testing::Return(polaris::ReturnCode::kReturnOk));

  register_info.name = callee_service;
  int ret = registry_->HeartBeat(&register_info);
  ASSERT_EQ(0, ret);
}

TEST_F(PolarisRegistryWithInstanceIdTest, AsyncHeartBeat) {
  polaris::ServiceKey service_key;
  PolarisHeartbeatCallback call_back(service_key);
  call_back.Response(polaris::ReturnCode::kReturnOk, "");
  call_back.Response(polaris::ReturnCode::kReturnUnknownError, "");

  std::string callee_service = "test.service";
  trpc::RegistryInfo register_info;
  register_info.meta["namespace"] = kPolarisNamespaceTest;

  std::string return_instance = "return_instance";
  EXPECT_CALL(*polaris::MockServerConnectorTest::server_connector_,
              AsyncInstanceHeartbeat(::testing::_, ::testing::_, ::testing::_))
      .Times(1)
      .WillRepeatedly(::testing::Return(polaris::ReturnCode::kReturnOk));

  register_info.name = callee_service;
  registry_->AsyncHeartBeat(&register_info)
      .Then([](trpc::Future<>&& registry_fut) {
        EXPECT_TRUE(registry_fut.IsReady());
        return trpc::MakeReadyFuture<>();
      })
      .Wait();
}

}  // namespace trpc

class PolarisTestEnvironment : public testing::Environment {
 public:
  virtual void SetUp() { trpc::TrpcPlugin::GetInstance()->RegisterPlugins(); }
  virtual void TearDown() { trpc::TrpcPlugin::GetInstance()->UnregisterPlugins(); }
};

int main(int argc, char** argv) {
  testing::AddGlobalTestEnvironment(new PolarisTestEnvironment);
  testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
