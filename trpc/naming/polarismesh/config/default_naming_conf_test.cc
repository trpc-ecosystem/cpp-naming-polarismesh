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

#include "trpc/naming/polarismesh/config/default_naming_conf.h"

#include <string>

#include "gtest/gtest.h"
#include "yaml-cpp/yaml.h"

namespace trpc {

TEST(DefaultNamingConfTest, default_naming_conf_test) {
  naming::ServiceConfig service_config;
  service_config.name = "test.serivce";
  service_config.namespace_ = "Test";
  service_config.token = "token";
  service_config.instance_id = "id";
  service_config.metadata = {{"key", "value"}, {"test", "metadata"}};
  service_config.Display();

  naming::RegistryConfig registry_config;
  registry_config.heartbeat_interval = 3333;
  registry_config.heartbeat_timeout = 2222;
  registry_config.services_config.push_back(service_config);
  registry_config.Display();

  YAML::convert<trpc::naming::RegistryConfig> c;
  YAML::Node config_node = c.encode(registry_config);

  naming::RegistryConfig tmp;
  ASSERT_TRUE(c.decode(config_node, tmp));

  tmp.Display();
  ASSERT_EQ(registry_config.heartbeat_interval, tmp.heartbeat_interval);
  ASSERT_EQ(registry_config.heartbeat_timeout, tmp.heartbeat_timeout);
  ASSERT_EQ(1, tmp.services_config.size());
  ASSERT_EQ(service_config.name, tmp.services_config[0].name);
  ASSERT_EQ(service_config.namespace_, tmp.services_config[0].namespace_);
  ASSERT_EQ(service_config.token, tmp.services_config[0].token);
  ASSERT_EQ(service_config.instance_id, tmp.services_config[0].instance_id);
  ASSERT_EQ(service_config.metadata, tmp.services_config[0].metadata);
}

}  // namespace trpc