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

#ifndef BUILD_EXCLUDE_NAMING_POLARIS
#include "trpc/naming/polarismesh/config/polaris_naming_conf.h"

#include <iostream>
#include <memory>
#include <string>

#include "gtest/gtest.h"
#include "yaml-cpp/yaml.h"

#include "trpc/naming/polarismesh/mock_polaris_api_test.h"

TEST(PolarisNamingConfig, Load) {
  trpc::PolarisNamingTestConfigSwitch defaultSwitch;
  defaultSwitch.bind_ip = "127.0.0.1";
  defaultSwitch.bind_if = "eth1";
  defaultSwitch.need_circuitbreaker = true;
  defaultSwitch.need_ratelimiter = true;
  std::string naming_config_str = trpc::buildPolarisNamingConfig(defaultSwitch);
  std::cout << "naming_config_str:" << std::endl << naming_config_str << std::endl;
  YAML::Node root = YAML::Load(naming_config_str);
  root["selector"]["polarismesh"]["global"]["system"]["healthCheckCluster"]["namespace"] = "Polaris";
  root["selector"]["polarismesh"]["global"]["system"]["healthCheckCluster"]["service"] = "polarismesh.healthcheck.pcg";
  root["selector"]["polarismesh"]["global"]["system"]["monitorCluster"]["namespace"] = "Polaris";
  root["selector"]["polarismesh"]["global"]["system"]["monitorCluster"]["service"] = "polarismesh.monitor.pcg";
  root["selector"]["polarismesh"]["consumer"]["circuitBreaker"]["setCircuitBreaker"]["enable"] = true;

  trpc::naming::PolarisNamingConfig naming_conf("polarismesh");
  ASSERT_TRUE(YAML::convert<trpc::naming::PolarisNamingConfig>::decode(root, naming_conf));
  naming_conf.Display();

  // Check whether the local network address is successfully changed
  ASSERT_EQ(defaultSwitch.bind_ip, naming_conf.selector_config.global_config.api_config.bind_ip);
  ASSERT_EQ(defaultSwitch.bind_if, naming_conf.selector_config.global_config.api_config.bind_if);

  // Verify that the service periodic refresh cycle is set correctly
  ASSERT_EQ(5055, naming_conf.selector_config.consumer_config.local_cache_config.service_refresh_interval);

  // Check whether the cluster is successfully set
  ASSERT_EQ("polarismesh.healthcheck.pcg",
            naming_conf.selector_config.global_config.system_config.clusters_config["healthCheckCluster"].service_name);
  ASSERT_EQ("polarismesh.monitor.pcg",
            naming_conf.selector_config.global_config.system_config.clusters_config["monitorCluster"].service_name);
  // Test whether the branch set fuse is open
  ASSERT_TRUE(naming_conf.selector_config.consumer_config.circuit_breaker_config.set_circuitbreaker_config.enable);
}

TEST(loadBalancerConfig, load_balance_config_test) {
  trpc::naming::LoadBalancerConfig load_balance_config;
  load_balance_config.type = "ringHash";
  load_balance_config.enable_dynamic_weight = false;
  load_balance_config.vnode_count = 1024;
  load_balance_config.compatible_golang = true;
  load_balance_config.Display();

  YAML::convert<trpc::naming::LoadBalancerConfig> c;
  YAML::Node config_node = c.encode(load_balance_config);

  trpc::naming::LoadBalancerConfig tmp;
  ASSERT_TRUE(c.decode(config_node, tmp));

  tmp.Display();
  ASSERT_EQ(load_balance_config.type, tmp.type);
  ASSERT_EQ(load_balance_config.enable_dynamic_weight, tmp.enable_dynamic_weight);
  ASSERT_EQ(load_balance_config.vnode_count, tmp.vnode_count);
  ASSERT_EQ(load_balance_config.compatible_golang, tmp.compatible_golang);
}

TEST(loadBalancerConfig, load_service_router_config_test) {
  // Configure the nearby route plug-in
  trpc::naming::NearbyBasedRouterConfig nearby_based_router_config;
  nearby_based_router_config.match_level = "campus";
  nearby_based_router_config.max_match_level = "region";
  nearby_based_router_config.strict_nearby = true;
  nearby_based_router_config.enable_degrade_by_unhealthy_percent = true;
  nearby_based_router_config.unhealthy_percent_to_degrade = 90;
  nearby_based_router_config.enable_recover_all = false;

  trpc::naming::RouterPluginConfig router_plugin_config;
  router_plugin_config.nearby_based_router_config = nearby_based_router_config;
  // Configure service routing
  trpc::naming::ServiceRouterConfig service_router_config;
  service_router_config.percent_of_min_instances = 1.0;
  service_router_config.enable = true;
  service_router_config.chain = {"ruleRouter", "nearbyRouter"};
  service_router_config.router_plugin_config = router_plugin_config;

  YAML::convert<trpc::naming::ServiceRouterConfig> c;
  YAML::Node config_node = c.encode(service_router_config);

  trpc::naming::ServiceRouterConfig tmp;
  ASSERT_TRUE(c.decode(config_node, tmp));

  tmp.Display();
  ASSERT_EQ(service_router_config.percent_of_min_instances, tmp.percent_of_min_instances);
  ASSERT_EQ(service_router_config.enable, tmp.enable);
  ASSERT_EQ(service_router_config.chain, tmp.chain);
  ASSERT_EQ(service_router_config.router_plugin_config.nearby_based_router_config.match_level,
            tmp.router_plugin_config.nearby_based_router_config.match_level);
  ASSERT_EQ(service_router_config.router_plugin_config.nearby_based_router_config.max_match_level,
            tmp.router_plugin_config.nearby_based_router_config.max_match_level);
  ASSERT_EQ(service_router_config.router_plugin_config.nearby_based_router_config.strict_nearby,
            tmp.router_plugin_config.nearby_based_router_config.strict_nearby);
  ASSERT_EQ(service_router_config.router_plugin_config.nearby_based_router_config.enable_degrade_by_unhealthy_percent,
            tmp.router_plugin_config.nearby_based_router_config.enable_degrade_by_unhealthy_percent);
  ASSERT_EQ(service_router_config.router_plugin_config.nearby_based_router_config.unhealthy_percent_to_degrade,
            tmp.router_plugin_config.nearby_based_router_config.unhealthy_percent_to_degrade);
  ASSERT_EQ(service_router_config.router_plugin_config.nearby_based_router_config.enable_recover_all,
            tmp.router_plugin_config.nearby_based_router_config.enable_recover_all);
}

TEST(ConsumerConfig, service_consumer_config_test) {
  trpc::naming::ConsumerConfig consumer_config;
  // Services configured with a special consumer module
  trpc::naming::ServiceConsumerConfig special_service_consumer_config;
  special_service_consumer_config.service_name = "special";
  special_service_consumer_config.service_namespace = "Test";
  special_service_consumer_config.set_circuit_breaker = true;
  special_service_consumer_config.circuit_breaker_config.enable = false;
  special_service_consumer_config.set_outlier_detection = true;
  special_service_consumer_config.outlier_detect_config.enable = true;
  special_service_consumer_config.set_load_balancer = true;
  special_service_consumer_config.load_balancer_config.type = "ringHash";
  special_service_consumer_config.set_service_router = true;
  special_service_consumer_config.service_router_config.enable = false;

  // Services configured with the default consumer module
  trpc::naming::ServiceConsumerConfig default_service_consumer_config;
  default_service_consumer_config.service_name = "default";
  default_service_consumer_config.service_namespace = "Test";

  consumer_config.service_consumer_config.push_back(special_service_consumer_config);
  consumer_config.service_consumer_config.push_back(default_service_consumer_config);

  YAML::convert<trpc::naming::ConsumerConfig> c;
  YAML::Node config_node = c.encode(consumer_config);

  trpc::naming::ConsumerConfig tmp;
  ASSERT_TRUE(c.decode(config_node, tmp));

  tmp.Display();
  ASSERT_EQ(2, tmp.service_consumer_config.size());
  ASSERT_EQ(special_service_consumer_config.service_name, tmp.service_consumer_config[0].service_name);
  ASSERT_EQ(special_service_consumer_config.service_namespace, tmp.service_consumer_config[0].service_namespace);
  ASSERT_EQ(special_service_consumer_config.set_circuit_breaker, tmp.service_consumer_config[0].set_circuit_breaker);
  ASSERT_EQ(special_service_consumer_config.circuit_breaker_config.enable,
            tmp.service_consumer_config[0].circuit_breaker_config.enable);
  ASSERT_EQ(special_service_consumer_config.set_outlier_detection,
            tmp.service_consumer_config[0].set_outlier_detection);
  ASSERT_EQ(special_service_consumer_config.outlier_detect_config.enable,
            tmp.service_consumer_config[0].outlier_detect_config.enable);
  ASSERT_EQ(special_service_consumer_config.set_service_router, tmp.service_consumer_config[0].set_service_router);
  ASSERT_EQ(special_service_consumer_config.service_router_config.enable,
            tmp.service_consumer_config[0].service_router_config.enable);

  ASSERT_EQ(default_service_consumer_config.service_name, tmp.service_consumer_config[1].service_name);
  ASSERT_EQ(default_service_consumer_config.service_namespace, tmp.service_consumer_config[1].service_namespace);
  ASSERT_EQ(default_service_consumer_config.set_circuit_breaker, false);
  ASSERT_EQ(default_service_consumer_config.set_outlier_detection, false);
  ASSERT_EQ(default_service_consumer_config.set_load_balancer, false);
  ASSERT_EQ(default_service_consumer_config.set_service_router, false);
}

#endif
