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

#include <stdio.h>
#include <vector>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/naming/common/common_defs.h"
namespace trpc {

TEST(TransResultTest, Ipv4) {
  std::vector<polaris::Instance> instances;
  polaris::Instance instance("1", "127.0.0.1", 10001, 0);
  instances.emplace_back(instance);
  std::vector<TrpcEndpointInfo> endpoints;

  ConvertPolarisInstances(instances, endpoints);
  ASSERT_EQ(1, endpoints.size());

  const TrpcEndpointInfo trpc_endpoint_info = *endpoints.cbegin();
  ASSERT_EQ("127.0.0.1", trpc_endpoint_info.host);
  ASSERT_EQ(10001, trpc_endpoint_info.port);
  ASSERT_EQ(false, trpc_endpoint_info.is_ipv6);
  ASSERT_EQ(0, trpc_endpoint_info.weight);
}

TEST(TransResultTest, Ipv6) {
  polaris::Instance instance("1", "::1", 10001, 100);
  TrpcEndpointInfo trpc_endpoint_info;
  ConvertPolarisInstance(instance, trpc_endpoint_info);
  ASSERT_EQ("::1", trpc_endpoint_info.host);
  ASSERT_EQ(10001, trpc_endpoint_info.port);
  ASSERT_EQ(true, trpc_endpoint_info.is_ipv6);
  ASSERT_EQ(100, trpc_endpoint_info.weight);
}

TEST(TransResultTest, TransMeta) {
  polaris::Instance instance("1", "127.0.0.1", 10001, 0);
  TrpcEndpointInfo endpoint_info_has_meta;
  ConvertPolarisInstance(instance, endpoint_info_has_meta);
  ASSERT_FALSE(endpoint_info_has_meta.meta.empty());
  ASSERT_EQ("1", endpoint_info_has_meta.meta["instance_id"]);

  TrpcEndpointInfo endpoint_info_no_meta;
  ConvertPolarisInstance(instance, endpoint_info_no_meta, false);
  ASSERT_TRUE(endpoint_info_no_meta.meta.empty());
}

TEST(ConvertToPolarisMeshRegistryInfoTest, Run) {
  RegistryInfo registry_info;
  registry_info.name = "test";
  registry_info.host = "127.0.0.1";
  registry_info.port = 1234;
  registry_info.meta["token"] = "token";
  registry_info.meta["instance_id"] = "instance_id";
  registry_info.meta["protocol"] = "grpc";
  registry_info.meta["weight"] = "200";
  registry_info.meta["priority"] = "1";
  registry_info.meta["version"] = "version";
  registry_info.meta["enable_health_check"] = "1";
  registry_info.meta["health_check_type"] = "1";
  registry_info.meta["ttl"] = "5";

  PolarisMeshRegistryInfo polaris_info;
  ConvertToPolarisMeshRegistryInfo(registry_info, polaris_info);
  ASSERT_EQ("test", polaris_info.service_name);
  ASSERT_EQ("127.0.0.1", polaris_info.host);
  ASSERT_EQ(1234, polaris_info.port);
  ASSERT_EQ("token", polaris_info.service_token);
  ASSERT_EQ("instance_id", polaris_info.instance_id);
  ASSERT_EQ("grpc", polaris_info.protocol);
  ASSERT_EQ(200, polaris_info.weight);
  ASSERT_EQ(1, polaris_info.priority);
  ASSERT_EQ("version", polaris_info.version);
  ASSERT_EQ(1, polaris_info.enable_health_check);
  ASSERT_EQ(1, polaris_info.health_check_type);
  ASSERT_EQ(5, polaris_info.ttl);
}

TEST(ServiceKeyTest, Compare) {
  polaris::ServiceKey service_key1 = {"Test", "service1"};
  polaris::ServiceKey service_key2 = {"Test", "service2"};
  ASSERT_EQ(true, ServiceKeyEqual(service_key1, service_key1));
  ASSERT_EQ(false, ServiceKeyEqual(service_key1, service_key2));
}

TEST(SetPolarisMeshSelectorConfTest, Run) {
  int ret = TrpcConfig::GetInstance()->Init("./trpc/naming/polarismesh/testing/polarismesh_test.yaml");
  ASSERT_EQ(0, ret);
  trpc::naming::PolarisMeshNamingConfig config;
  SetPolarisMeshSelectorConf(config);
  ASSERT_NE(0, config.orig_selector_config.size());
}

TEST(FrameworkRetToPolarisRet, Convert) {
  ReadersWriterData<std::set<int>> whitelist;
  whitelist.Writer().insert(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR);
  whitelist.Swap();
  ASSERT_EQ(FrameworkRetToPolarisRet(whitelist, TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR),
            polaris::CallRetStatus::kCallRetOk);
  ASSERT_EQ(FrameworkRetToPolarisRet(whitelist, TrpcRetCode::TRPC_INVOKE_SUCCESS), polaris::CallRetStatus::kCallRetOk);
  ASSERT_EQ(FrameworkRetToPolarisRet(whitelist, TrpcRetCode::TRPC_CLIENT_CONNECT_ERR),
            polaris::CallRetStatus::kCallRetTimeout);
  ASSERT_EQ(FrameworkRetToPolarisRet(whitelist, TrpcRetCode::TRPC_CLIENT_INVOKE_TIMEOUT_ERR),
            polaris::CallRetStatus::kCallRetTimeout);
  ASSERT_EQ(FrameworkRetToPolarisRet(whitelist, TrpcRetCode::TRPC_CLIENT_FULL_LINK_TIMEOUT_ERR),
            polaris::CallRetStatus::kCallRetTimeout);
  ASSERT_EQ(FrameworkRetToPolarisRet(whitelist, TrpcRetCode::TRPC_CLIENT_CANCELED_ERR),
            polaris::CallRetStatus::kCallRetError);

  // 重新设置
  whitelist.Writer().clear();
  whitelist.Writer().insert(TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR);
  whitelist.Swap();
  ASSERT_EQ(FrameworkRetToPolarisRet(whitelist, TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR),
            polaris::CallRetStatus::kCallRetError);
  ASSERT_EQ(FrameworkRetToPolarisRet(whitelist, TrpcRetCode::TRPC_INVOKE_UNKNOWN_ERR),
            polaris::CallRetStatus::kCallRetOk);
}

}  // namespace trpc

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
