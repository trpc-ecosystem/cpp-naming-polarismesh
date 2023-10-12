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

#pragma once

#include <map>
#include <string>
#include <vector>
#include <iostream>
#include "trpc/naming/polarismesh/config/default_naming_conf.h"
#include "yaml-cpp/yaml.h"

namespace trpc::naming {

// Client cmdb location information
struct LocationConfig {
  std::string region;        // regionPolarisMeshNamingConfig
  std::string zone;          // zone
  std::string campus;        // campus
  bool enable_update{true};  // Whether to update location information from the CMDB

  void Display() const;
};

struct ServiceClusterConfig {
  std::string service_namespace;
  std::string service_name;

  void Display() const;
};

struct SystemConfig {
  std::map<std::string, ServiceClusterConfig> clusters_config;

  void Display() const;
};

struct ApiConfig {
  std::string bind_ip;  // Specify the local ip address. If the user does not specify the local IP address, use the
                        // local_ip address in the configuration file
  std::string bind_if;  // Specifies the ip address obtained from a network adapter
  uint64_t report_interval{600000};  // Set the interval for retrieving local region information
  LocationConfig location_config;

  void Display() const;
};

constexpr uint64_t kServiceRefreshInterval = 10000;  // The default service is refreshed periodically

struct ServerConnectorConfig {
  uint64_t timeout{2000};                              // Service discovery timeout period
  std::string join_point{"default"};                   // The polarismesh access point
  std::string protocol{"grpc"};                        // Service discovery protocol used
  uint64_t refresh_interval{kServiceRefreshInterval};  // The service is refreshed periodically
  std::vector<std::string> addresses;  // polarismesh Access layer native buried site address (grpc protocol use)
  uint64_t server_switch_interval{
      600000};  // Polaris service discovery server switching cycle (sdk default value 10 minutes)

  void Display() const;
};

struct ServerMetricConfig {
  std::string name{"trpc"};
  bool enable{true};
  std::string metrics_name;

  void Display() const;
};

struct GlobalConfig {
  SystemConfig system_config;
  ApiConfig api_config;
  ServerConnectorConfig server_connector_config;
  ServerMetricConfig server_metric_config;

  void Display() const;
};

struct LocalCacheConfig {
  std::string type{"inmemory"};                                        // Cache type
  std::string service_expire_time{"24h"};                              // Cache expiration time
  uint64_t service_refresh_interval{kServiceRefreshInterval};          // Service regular refresh cycle
  std::string persist_dir{"/usr/local/trpc/data/polarismesh/backup"};  // File cache directory

  void Display() const;
};

// polarismesh Error Follow Configuration
struct ErrorCountConfig {
  int continuous_error_threshold{10};  // Trigger a threshold for continuous error melting
  uint64_t sleep_window{30000};  // After the fuse is opened, how long will it be converted to a semi -open state later
  int request_count_after_halfopen{10};   // The maximum allowable request after the fuse is half open
  int success_count_after_halfopen{8};    // The minimum required required request for the fuse to the closure
  uint64_t metric_expired_time{3600000};  // Follow statistical sliding window expiration time

  // Print information
  void Display() const;
};

// polarismesh error rate fusion configuration
struct ErrorRateConfig {
  int request_volume_threshold{10};         // The minimum request threshold of trigger error rate fuse
  float error_rate_threshold{0.5};          // Threshold for triggering error rate fuse
  uint64_t metric_stat_time_window{60000};  // Statistical cycle of error rate fusion
  int metric_num_buckets{12};               // The minimum statistical unit of error rate fuse
  uint64_t sleep_window{30000};  // After the fuse is opened, how long will it be converted to a semi -open state later
  int request_count_after_halfopen{10};   // The maximum allowable request after the fuse is half open
  int success_count_after_halfopen{8};    // The minimum required required request for the fuse to the closure
  uint64_t metric_expired_time{3600000};  // Follow statistical sliding window expiration time

  // Print information
  void Display() const;
};

struct CircuitBreakerPluginConfig {
  ErrorCountConfig error_count_config;  // Error melting plug -in
  ErrorRateConfig error_rate_config;    // Error melting plug -in

  // Print information
  void Display() const;
};

// polarismesh Set Set Slim Freachment Configuration
struct SetCircuitBreakerConfig {
  bool enable{false};  // Whether to open the function

  void Display() const;
};

// Break -related configuration
struct CircuitBreakerConfig {
  bool enable{true};                                          // Whether to enable melting
  std::string check_period{"500"};                            // Interval between melting detection
  std::vector<std::string> chain{"errorCount", "errorRate"};  // The fusion plug -in used
  CircuitBreakerPluginConfig plugin_config;                   // Frameless plug -in configuration
  SetCircuitBreakerConfig set_circuitbreaker_config;          // Divided SET fuse function configuration
  void Display() const;
};

// Detective configuration
struct OutlierDetectionConfig {
  bool enable{false};
  std::string check_period{"10s"};
  std::vector<std::string> chain{"tcp"};
  void Display() const;
};

struct LoadBalancerConfig {
  std::string type{"weightedRandom"};
  // Dynamic weight routing is used: indicates whether the weight random algorithm starts the dynamic weight
  bool enable_dynamic_weight{false};
  // The number of virtual nodes of the hash ring algorithm, which is clearly set to> 0 to transmit it to SDK
  uint32_t vnode_count{0};
  // Indicates whether the result of the Arctic CPP SDK load balancing algorithm needs to be consistent with the Golang
  // SDK. This item is clearly set to the real transparency to the SDK
  bool compatible_golang{false};
  void Display() const;
};

// Related configuration of nearby routing plug -in
struct NearbyBasedRouterConfig {
  // As for the near -level keywords and scope interpretations, NONE (National)> Region (Region)> Zone (City)> Campus
  // (Park) The minimum matching level of nearby routing
  std::string match_level{"zone"};
  // Just the maximum matching level near the route
  std::string max_match_level{"none"};
  // Whether it is strict near, after opening, you must get the main position information from the server before it can
  // be executed.
  bool strict_nearby{false};
  // Whether to enable the proportion of unhealthy services to downgrade, the scope of the proportion statistics is an
  // instance episode of the current matching level
  bool enable_degrade_by_unhealthy_percent{true};
  // The proportion of instances that need to be downgraded.
  int unhealthy_percent_to_degrade{100};
  // Whether to allow full death
  bool enable_recover_all{true};
  void Display() const;
};

// Service routing plug -in related configuration
struct RouterPluginConfig {
  NearbyBasedRouterConfig nearby_based_router_config;
  void Display() const;
};

// Service routing related configuration
struct ServiceRouterConfig {
  float percent_of_min_instances{0.0};
  bool enable{true};
  std::vector<std::string> chain{"ruleRouter", "setDivisionRouter", "nearbyRouter", "canaryRouter"};
  RouterPluginConfig router_plugin_config;
  void Display() const;
};

// Service grade Consumer module configuration
struct ServiceConsumerConfig {
  // Benevolence
  std::string service_name;
  // Namespaces
  std::string service_namespace;
  // Whether to set up service -grade melting configuration
  bool set_circuit_breaker{false};
  // Service -grade melting configuration
  CircuitBreakerConfig circuit_breaker_config;
  // Whether to set service -level detection configuration
  bool set_outlier_detection{false};
  // Service -level detection configuration
  OutlierDetectionConfig outlier_detect_config;
  // Whether to set service -grade load balancing plug -in initialization configuration
  bool set_load_balancer{false};
  // Initialization configuration of service -grade load balancing plug -in
  LoadBalancerConfig load_balancer_config;
  // Whether to set up a service -level routing call chain configuration
  bool set_service_router{false};
  // Service -level route call chain configuration
  ServiceRouterConfig service_router_config;
  // Print information
  void Display() const;
};

// Fortune -level Consumer module configuration
struct ConsumerConfig {
  // Cache file configuration
  LocalCacheConfig local_cache_config;
  // Global melting configuration
  CircuitBreakerConfig circuit_breaker_config;
  // Global -level detection configuration
  OutlierDetectionConfig outlier_detect_config;
  // Global load balancing plug -in initialization configuration
  LoadBalancerConfig load_balancer_config;
  // Global routing call chain configuration
  ServiceRouterConfig service_router_config;
  // The service -grade Consumer module configuration collection, which is adjusted by the polarismesh service name
  // without service -grade configuration, using global configuration
  std::vector<ServiceConsumerConfig> service_consumer_config;
  // The TRPC protocol transmission field is passed to the polarismesh for the switch used by Meta matching
  bool enable_trans_meta{false};
  // Print information
  void Display() const;
};

// Dynamic weight module configuration
struct DynamicWeightConfig {
  // The dynamic weight process needs to occupy a single thread, and the opening indicates that starting the
  // corresponding thread
  bool open_dynamic_weight = false;

  // Print information
  void Display() const;
};

// Route Select Configuration
struct SelectorConfig {
  GlobalConfig global_config;
  ConsumerConfig consumer_config;
  DynamicWeightConfig dynamic_weight_config;

  // Print information
  void Display() const;
};

// Visit current -limiting configuration
struct RateLimiterConfig {
  // Query timeout of the current, 1000ms by default
  uint32_t timeout = 1000;
  // Report the results of the request to deal with the dynamic threshold adjustment of the polarismesh for the current
  // -limiting rules, and close it by default
  bool update_call_result = false;
  // Flow limit mode, default is the global mode
  std::string mode = "global";
  // polarismesh Limited Flowing Cluster Configuration
  ServiceClusterConfig cluster_config;

  // Print information
  void Display() const;
};

struct PolarisMeshNamingConfig {
  PolarisMeshNamingConfig() {}
  explicit PolarisMeshNamingConfig(const std::string& name) { this->name = name; }
  std::string name;
  RegistryConfig registry_config;
  SelectorConfig selector_config;
  RateLimiterConfig ratelimiter_config;
  std::string orig_selector_config;

  // Print information
  void Display() const;
};

}  // namespace trpc::naming

namespace YAML {

template <>
struct convert<trpc::naming::ServiceClusterConfig> {
  static YAML::Node encode(const trpc::naming::ServiceClusterConfig& config) {
    YAML::Node node;

    node["namespace"] = config.service_namespace;

    node["service"] = config.service_name;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::ServiceClusterConfig& config) {
    if (node["namespace"]) {
      config.service_namespace = node["namespace"].as<std::string>();
    }

    if (node["service"]) {
      config.service_name = node["service"].as<std::string>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::SystemConfig> {
  static YAML::Node encode(const trpc::naming::SystemConfig& config) {
    YAML::Node node;

    auto iter = config.clusters_config.find("discoverCluster");
    if (iter != config.clusters_config.end()) {
      node["discoverCluster"] = iter->second;
    }

    iter = config.clusters_config.find("healthCheckCluster");
    if (iter != config.clusters_config.end()) {
      node["healthCheckCluster"] = iter->second;
    }

    iter = config.clusters_config.find("monitorCluster");
    if (iter != config.clusters_config.end()) {
      node["monitorCluster"] = iter->second;
    }

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::SystemConfig& config) {
    if (node["discoverCluster"]) {
      config.clusters_config["discoverCluster"] = node["discoverCluster"].as<trpc::naming::ServiceClusterConfig>();
    }

    if (node["healthCheckCluster"]) {
      config.clusters_config["healthCheckCluster"] =
          node["healthCheckCluster"].as<trpc::naming::ServiceClusterConfig>();
    }

    if (node["monitorCluster"]) {
      config.clusters_config["monitorCluster"] = node["monitorCluster"].as<trpc::naming::ServiceClusterConfig>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::LocationConfig> {
  static YAML::Node encode(const trpc::naming::LocationConfig& config) {
    YAML::Node node;

    node["region"] = config.region;

    node["zone"] = config.zone;

    node["campus"] = config.campus;

    node["enableUpdate"] = config.enable_update;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::LocationConfig& config) {
    if (node["region"]) {
      config.region = node["region"].as<std::string>();
    }

    if (node["zone"]) {
      config.zone = node["zone"].as<std::string>();
    }

    if (node["campus"]) {
      config.campus = node["campus"].as<std::string>();
    }

    if (node["enableUpdate"]) {
      config.enable_update = node["enableUpdate"].as<bool>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::ApiConfig> {
  static YAML::Node encode(const trpc::naming::ApiConfig& config) {
    YAML::Node node;

    node["bindIP"] = config.bind_ip;

    node["bindIf"] = config.bind_if;

    node["reportInterval"] = config.report_interval;

    node["location"] = config.location_config;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::ApiConfig& config) {
    if (node["bindIP"]) {
      // If you are not set, you will use the local_ip in the configuration file (compatible with the old version logic)
      config.bind_ip = node["bindIP"].as<std::string>();
    }

    if (node["bindIf"]) {
      config.bind_if = node["bindIf"].as<std::string>();
    }

    if (node["reportInterval"]) {
      config.report_interval = node["reportInterval"].as<uint64_t>();
    }

    if (node["location"]) {
      config.location_config = node["location"].as<trpc::naming::LocationConfig>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::ServerConnectorConfig> {
  static YAML::Node encode(const trpc::naming::ServerConnectorConfig& config) {
    YAML::Node node;

    node["timeout"] = config.timeout;

    node["messageTimeout"] = config.timeout;

    node["joinPoint"] = config.join_point;

    node["protocol"] = config.protocol;

    node["addresses"] = config.addresses;

    node["serverSwitchInterval"] = config.server_switch_interval;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::ServerConnectorConfig& config) {
    if (node["timeout"]) {
      config.timeout = node["timeout"].as<uint64_t>();
    }

    if (node["joinPoint"]) {
      config.join_point = node["joinPoint"].as<std::string>();
    }

    if (node["protocol"]) {
      config.protocol = node["protocol"].as<std::string>();
    }

    if (node["refresh_interval"]) {
      config.refresh_interval = node["refresh_interval"].as<uint64_t>();
    }

    if (node["addresses"]) {
      config.addresses = node["addresses"].as<std::vector<std::string>>();
    }

    if (node["serverSwitchInterval"]) {
      config.server_switch_interval = node["serverSwitchInterval"].as<uint64_t>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::ServerMetricConfig> {
  static YAML::Node encode(const trpc::naming::ServerMetricConfig& config) {
    YAML::Node node;

    node["name"] = config.name;

    node["enable"] = config.enable;

    node["metrics_name"] = config.metrics_name;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::ServerMetricConfig& config) {
    if (node["enable"]) {
      config.enable = node["enable"].as<bool>();
    }

    if (node["metrics_name"]) {
      config.metrics_name = node["metrics_name"].as<std::string>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::GlobalConfig> {
  static YAML::Node encode(const trpc::naming::GlobalConfig& config) {
    YAML::Node node;

    if (!config.system_config.clusters_config.empty()) {
      node["system"] = config.system_config;
    }

    node["api"] = config.api_config;

    node["serverConnector"] = config.server_connector_config;

    node["serverMetric"] = config.server_metric_config;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::GlobalConfig& config) {
    if (node["system"]) {
      config.system_config = node["system"].as<trpc::naming::SystemConfig>();
    }

    if (node["api"]) {
      config.api_config = node["api"].as<trpc::naming::ApiConfig>();
    }

    if (node["serverConnector"]) {
      config.server_connector_config = node["serverConnector"].as<trpc::naming::ServerConnectorConfig>();
    }

    if (node["serverMetric"]) {
      config.server_metric_config = node["serverMetric"].as<trpc::naming::ServerMetricConfig>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::LocalCacheConfig> {
  static YAML::Node encode(const trpc::naming::LocalCacheConfig& config) {
    YAML::Node node;

    node["type"] = config.type;

    node["serviceExpireTime"] = config.service_expire_time;

    node["serviceRefreshInterval"] = config.service_refresh_interval;

    node["persistDir"] = config.persist_dir;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::LocalCacheConfig& config) {
    if (node["type"]) {
      config.type = node["type"].as<std::string>();
    }

    if (node["serviceExpireTime"]) {
      config.service_expire_time = node["serviceExpireTime"].as<std::string>();
    }

    if (node["serviceRefreshInterval"]) {
      config.service_refresh_interval = node["serviceRefreshInterval"].as<uint64_t>();
    }

    if (node["persistDir"]) {
      config.persist_dir = node["persistDir"].as<std::string>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::ServiceConsumerConfig> {
  static YAML::Node encode(const trpc::naming::ServiceConsumerConfig& config) {
    YAML::Node node;

    node["name"] = config.service_name;
    node["namespace"] = config.service_namespace;

    if (config.set_circuit_breaker) {
      node["circuitBreaker"] = config.circuit_breaker_config;
    }

    if (config.set_outlier_detection) {
      node["outlierDetection"] = config.outlier_detect_config;
    }

    // Historical issues, TRPC's YAML configuration business habit fills LoadBalancer, but it needs to be transmitted to
    // the polarismesh and needs to be converted to LoadBarancer
    if (config.set_load_balancer) {
      node["loadBalancer"] = config.load_balancer_config;
    }

    if (config.set_service_router) {
      node["serviceRouter"] = config.service_router_config;
    }

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::ServiceConsumerConfig& config) {
    if (node["name"]) {
      config.service_name = node["name"].as<std::string>();
    }

    if (node["namespace"]) {
      config.service_namespace = node["namespace"].as<std::string>();
    }

    if (node["circuitBreaker"]) {
      config.set_circuit_breaker = true;
      config.circuit_breaker_config = node["circuitBreaker"].as<trpc::naming::CircuitBreakerConfig>();
    }

    if (node["outlierDetection"]) {
      config.set_outlier_detection = true;
      config.outlier_detect_config = node["outlierDetection"].as<trpc::naming::OutlierDetectionConfig>();
    }

    if (node["loadBalancer"]) {
      config.set_load_balancer = true;
      config.load_balancer_config = node["loadBalancer"].as<trpc::naming::LoadBalancerConfig>();
    }

    if (node["serviceRouter"]) {
      config.set_service_router = true;
      config.service_router_config = node["serviceRouter"].as<trpc::naming::ServiceRouterConfig>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::ConsumerConfig> {
  static YAML::Node encode(const trpc::naming::ConsumerConfig& config) {
    YAML::Node node;

    node["localCache"] = config.local_cache_config;
    node["circuitBreaker"] = config.circuit_breaker_config;
    node["outlierDetection"] = config.outlier_detect_config;
    // Historical issues, TRPC's YAML configuration business habit fills Loadbarancer, but it needs to be transmitted to
    // the polarismesh and needs to be converted to LoadBarancer
    node["loadBalancer"] = config.load_balancer_config;
    node["serviceRouter"] = config.service_router_config;
    node["service"] = config.service_consumer_config;
    node["enableTransMeta"] = config.enable_trans_meta;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::ConsumerConfig& config) {
    if (node["localCache"]) {
      config.local_cache_config = node["localCache"].as<trpc::naming::LocalCacheConfig>();
    }

    if (node["circuitBreaker"]) {
      config.circuit_breaker_config = node["circuitBreaker"].as<trpc::naming::CircuitBreakerConfig>();
    }

    if (node["outlierDetection"]) {
      config.outlier_detect_config = node["outlierDetection"].as<trpc::naming::OutlierDetectionConfig>();
    }

    if (node["loadBalancer"]) {
      config.load_balancer_config = node["loadBalancer"].as<trpc::naming::LoadBalancerConfig>();
    }

    if (node["serviceRouter"]) {
      config.service_router_config = node["serviceRouter"].as<trpc::naming::ServiceRouterConfig>();
    }

    auto service = node["service"];
    if (service) {
      for (auto&& idx : service) {
        config.service_consumer_config.push_back(idx.as<trpc::naming::ServiceConsumerConfig>());
      }
    }

    if (node["enableTransMeta"]) {
      config.enable_trans_meta = node["enableTransMeta"].as<bool>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::DynamicWeightConfig> {
  static YAML::Node encode(const trpc::naming::DynamicWeightConfig& config) {
    YAML::Node node;

    node["isOpenDynamicWeight"] = config.open_dynamic_weight;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::DynamicWeightConfig& config) {
    if (node["isOpenDynamicWeight"]) {
      config.open_dynamic_weight = node["isOpenDynamicWeight"].as<bool>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::SelectorConfig> {
  static YAML::Node encode(const trpc::naming::SelectorConfig& config) {
    YAML::Node node;

    node["global"] = config.global_config;

    node["consumer"] = config.consumer_config;

    node["dynamic_weight"] = config.dynamic_weight_config;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::SelectorConfig& config) {
    if (node["global"]) {
      config.global_config = node["global"].as<trpc::naming::GlobalConfig>();
    }

    if (node["consumer"]) {
      config.consumer_config = node["consumer"].as<trpc::naming::ConsumerConfig>();

      // Compatible with the old configuration, set the service regular refresh cycle
      if (config.consumer_config.local_cache_config.service_refresh_interval == trpc::naming::kServiceRefreshInterval &&
          config.global_config.server_connector_config.refresh_interval != trpc::naming::kServiceRefreshInterval) {
        config.consumer_config.local_cache_config.service_refresh_interval =
            config.global_config.server_connector_config.refresh_interval;
      }
    }

    if (node["dynamic_weight"]) {
      config.dynamic_weight_config = node["dynamic_weight"].as<trpc::naming::DynamicWeightConfig>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::RateLimiterConfig> {
  static YAML::Node encode(const trpc::naming::RateLimiterConfig& config) {
    YAML::Node node;
    node["timeout"] = config.timeout;
    node["updateCallResult"] = config.update_call_result;
    node["mode"] = config.mode;
    node["rateLimitCluster"] = config.cluster_config;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::RateLimiterConfig& config) {
    if (node["timeout"]) {
      config.timeout = node["timeout"].as<uint32_t>();
    }

    if (node["updateCallResult"]) {
      config.update_call_result = node["updateCallResult"].as<bool>();
    }

    if (node["mode"]) {
      config.mode = node["mode"].as<std::string>();
    }

    if (node["rateLimitCluster"]) {
      config.cluster_config = node["rateLimitCluster"].as<trpc::naming::ServiceClusterConfig>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::PolarisMeshNamingConfig> {
  static YAML::Node encode(const trpc::naming::PolarisMeshNamingConfig& config) {
    YAML::Node node;

    node["registry"][config.name] = config.registry_config;

    node["selector"][config.name] = config.selector_config;

    node["limiter"][config.name] = config.ratelimiter_config;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::PolarisMeshNamingConfig& config) {
    if (node["registry"] && node["registry"][config.name]) {
      config.registry_config = node["registry"][config.name].as<trpc::naming::RegistryConfig>();
    }

    if (node["selector"] && node["selector"][config.name]) {
      config.selector_config = node["selector"][config.name].as<trpc::naming::SelectorConfig>();
      std::stringstream strstream;
      strstream << node["selector"][config.name];
      std::string orig_selector_config = strstream.str();
      config.orig_selector_config = orig_selector_config;
    }

    if (node["limiter"] && node["limiter"][config.name]) {
      config.ratelimiter_config = node["limiter"][config.name].as<trpc::naming::RateLimiterConfig>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::LoadBalancerConfig> {
  static YAML::Node encode(const trpc::naming::LoadBalancerConfig& config) {
    YAML::Node node;
    node["type"] = config.type;

    node["enableDynamicWeight"] = config.enable_dynamic_weight;

    if (config.vnode_count > 0) {
      node["vnodeCount"] = config.vnode_count;
    }

    if (config.compatible_golang) {
      node["compatibleGo"] = config.compatible_golang;
    }

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::LoadBalancerConfig& config) {
    if (node["type"]) {
      config.type = node["type"].as<std::string>();
    }

    if (node["enableDynamicWeight"]) {
      config.enable_dynamic_weight = node["enableDynamicWeight"].as<bool>();
    }

    if (node["vnodeCount"]) {
      config.vnode_count = node["vnodeCount"].as<uint32_t>();
    }

    if (node["compatibleGo"]) {
      config.compatible_golang = node["compatibleGo"].as<bool>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::OutlierDetectionConfig> {
  static YAML::Node encode(const trpc::naming::OutlierDetectionConfig& config) {
    YAML::Node node;
    node["enable"] = config.enable;
    node["checkPeriod"] = config.check_period;
    node["chain"] = config.chain;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::OutlierDetectionConfig& config) {
    if (node["enable"]) {
      config.enable = node["enable"].as<bool>();
    }
    if (node["checkPeriod"]) {
      config.check_period = node["checkPeriod"].as<std::string>();
    }
    if (node["chain"]) {
      config.chain = node["chain"].as<std::vector<std::string>>();
    }
    return true;
  }
};

template <>
struct convert<trpc::naming::NearbyBasedRouterConfig> {
  static YAML::Node encode(const trpc::naming::NearbyBasedRouterConfig& config) {
    YAML::Node node;
    node["matchLevel"] = config.match_level;
    node["maxMatchLevel"] = config.max_match_level;
    node["strictNearby"] = config.strict_nearby;
    node["enableDegradeByUnhealthyPercent"] = config.enable_degrade_by_unhealthy_percent;
    node["unhealthyPercentToDegrade"] = config.unhealthy_percent_to_degrade;
    node["enableRecoverAll"] = config.enable_recover_all;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::NearbyBasedRouterConfig& config) {
    if (node["matchLevel"]) {
      config.match_level = node["matchLevel"].as<std::string>();
    }
    if (node["maxMatchLevel"]) {
      config.max_match_level = node["maxMatchLevel"].as<std::string>();
    }
    if (node["strictNearby"]) {
      config.strict_nearby = node["strictNearby"].as<bool>();
    }
    if (node["enableDegradeByUnhealthyPercent"]) {
      config.enable_degrade_by_unhealthy_percent = node["enableDegradeByUnhealthyPercent"].as<bool>();
    }
    if (node["unhealthyPercentToDegrade"]) {
      config.unhealthy_percent_to_degrade = node["unhealthyPercentToDegrade"].as<int>();
    }
    if (node["enableRecoverAll"]) {
      config.enable_recover_all = node["enableRecoverAll"].as<bool>();
    }
    return true;
  }
};

template <>
struct convert<trpc::naming::RouterPluginConfig> {
  static YAML::Node encode(const trpc::naming::RouterPluginConfig& config) {
    YAML::Node node;
    node["nearbyBasedRouter"] = config.nearby_based_router_config;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::RouterPluginConfig& config) {
    if (node["nearbyBasedRouter"]) {
      config.nearby_based_router_config = node["nearbyBasedRouter"].as<trpc::naming::NearbyBasedRouterConfig>();
    }
    return true;
  }
};

template <>
struct convert<trpc::naming::ServiceRouterConfig> {
  static YAML::Node encode(const trpc::naming::ServiceRouterConfig& config) {
    YAML::Node node;
    node["enable"] = config.enable;
    node["percentOfMinInstances"] = config.percent_of_min_instances;
    node["chain"] = config.chain;
    node["plugin"] = config.router_plugin_config;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::ServiceRouterConfig& config) {
    if (node["enable"]) {
      config.enable = node["enable"].as<bool>();
    }
    if (node["percentOfMinInstances"]) {
      config.percent_of_min_instances = node["percentOfMinInstances"].as<float>();
    }
    if (node["chain"]) {
      config.chain = node["chain"].as<std::vector<std::string>>();
    }
    if (node["plugin"]) {
      config.router_plugin_config = node["plugin"].as<trpc::naming::RouterPluginConfig>();
    }
    return true;
  }
};

template <>
struct convert<trpc::naming::ErrorCountConfig> {
  static YAML::Node encode(const trpc::naming::ErrorCountConfig& config) {
    YAML::Node node;
    node["continuousErrorThreshold"] = config.continuous_error_threshold;
    node["sleepWindow"] = config.sleep_window;
    node["requestCountAfterHalfOpen"] = config.request_count_after_halfopen;
    node["successCountAfterHalfOpen"] = config.success_count_after_halfopen;
    node["metricExpiredTime"] = config.metric_expired_time;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::ErrorCountConfig& config) {
    if (node["continuousErrorThreshold"]) {
      config.continuous_error_threshold = node["continuousErrorThreshold"].as<int>();
    }
    if (node["sleepWindow"]) {
      config.sleep_window = node["sleepWindow"].as<uint64_t>();
    }
    if (node["requestCountAfterHalfOpen"]) {
      config.request_count_after_halfopen = node["requestCountAfterHalfOpen"].as<int>();
    }
    if (node["successCountAfterHalfOpen"]) {
      config.success_count_after_halfopen = node["successCountAfterHalfOpen"].as<int>();
    }
    if (node["metricExpiredTime"]) {
      config.metric_expired_time = node["metricExpiredTime"].as<uint64_t>();
    }
    return true;
  }
};

template <>
struct convert<trpc::naming::ErrorRateConfig> {
  static YAML::Node encode(const trpc::naming::ErrorRateConfig& config) {
    YAML::Node node;
    node["requestVolumeThreshold"] = config.request_volume_threshold;
    node["errorRateThreshold"] = config.error_rate_threshold;
    node["metricStatTimeWindow"] = config.metric_stat_time_window;
    node["metricNumBuckets"] = config.metric_num_buckets;
    node["sleepWindow"] = config.sleep_window;
    node["requestCountAfterHalfOpen"] = config.request_count_after_halfopen;
    node["successCountAfterHalfOpen"] = config.success_count_after_halfopen;
    node["metricExpiredTime"] = config.metric_expired_time;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::ErrorRateConfig& config) {
    if (node["requestVolumeThreshold"]) {
      config.request_volume_threshold = node["requestVolumeThreshold"].as<int>();
    }
    if (node["errorRateThreshold"]) {
      config.error_rate_threshold = node["errorRateThreshold"].as<float>();
    }
    if (node["metricStatTimeWindow"]) {
      config.metric_stat_time_window = node["metricStatTimeWindow"].as<uint64_t>();
    }
    if (node["metricNumBuckets"]) {
      config.metric_num_buckets = node["metricNumBuckets"].as<int>();
    }
    if (node["sleepWindow"]) {
      config.sleep_window = node["sleepWindow"].as<uint64_t>();
    }
    if (node["requestCountAfterHalfOpen"]) {
      config.request_count_after_halfopen = node["requestCountAfterHalfOpen"].as<int>();
    }
    if (node["successCountAfterHalfOpen"]) {
      config.success_count_after_halfopen = node["successCountAfterHalfOpen"].as<int>();
    }
    if (node["metricExpiredTime"]) {
      config.metric_expired_time = node["metricExpiredTime"].as<uint64_t>();
    }
    return true;
  }
};

template <>
struct convert<trpc::naming::CircuitBreakerPluginConfig> {
  static YAML::Node encode(const trpc::naming::CircuitBreakerPluginConfig& config) {
    YAML::Node node;
    node["errorCount"] = config.error_count_config;
    node["errorRate"] = config.error_rate_config;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::CircuitBreakerPluginConfig& config) {
    if (node["errorCount"]) {
      config.error_count_config = node["errorCount"].as<trpc::naming::ErrorCountConfig>();
    }
    if (node["errorRate"]) {
      config.error_rate_config = node["errorRate"].as<trpc::naming::ErrorRateConfig>();
    }
    return true;
  }
};

template <>
struct convert<trpc::naming::SetCircuitBreakerConfig> {
  static YAML::Node encode(const trpc::naming::SetCircuitBreakerConfig& config) {
    YAML::Node node;
    node["enable"] = config.enable;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::SetCircuitBreakerConfig& config) {
    if (node["enable"]) {
      config.enable = node["enable"].as<bool>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::CircuitBreakerConfig> {
  static YAML::Node encode(const trpc::naming::CircuitBreakerConfig& config) {
    YAML::Node node;
    node["enable"] = config.enable;
    node["chain"] = config.chain;
    node["checkPeriod"] = config.check_period;
    node["plugin"] = config.plugin_config;
    node["setCircuitBreaker"] = config.set_circuitbreaker_config;
    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::CircuitBreakerConfig& config) {
    if (node["enable"]) {
      config.enable = node["enable"].as<bool>();
    }
    if (node["chain"]) {
      config.chain = node["chain"].as<std::vector<std::string>>();
    }
    if (node["checkPeriod"]) {
      config.check_period = node["checkPeriod"].as<std::string>();
    }
    if (node["plugin"]) {
      config.plugin_config = node["plugin"].as<trpc::naming::CircuitBreakerPluginConfig>();
    }
    if (node["setCircuitBreaker"]) {
      config.set_circuitbreaker_config = node["setCircuitBreaker"].as<trpc::naming::SetCircuitBreakerConfig>();
    }
    return true;
  }
};

}  // namespace YAML
