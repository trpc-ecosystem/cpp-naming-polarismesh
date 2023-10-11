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

#include "trpc/naming/polarismesh/config/polarismesh_naming_conf.h"
#include "trpc/util/log/logging.h"

namespace trpc::naming {

void ServiceClusterConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("namespace:" << service_namespace);
  TRPC_LOG_DEBUG("service:" << service_name);

  TRPC_LOG_DEBUG("--------------------------------");
}

void SystemConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  for (auto& item : clusters_config) {
    TRPC_LOG_DEBUG("Cluster name:" << item.first);
    item.second.Display();
  }

  TRPC_LOG_DEBUG("--------------------------------");
}

void LocationConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("region:" << region);
  TRPC_LOG_DEBUG("zone:" << zone);
  TRPC_LOG_DEBUG("campus:" << campus);
  TRPC_LOG_DEBUG("enableUpdate:" << enable_update);

  TRPC_LOG_DEBUG("--------------------------------");
}

void ApiConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("bind_ip:" << bind_ip);
  TRPC_LOG_DEBUG("bind_if:" << bind_if);
  TRPC_LOG_DEBUG("reportInterval:" << report_interval);
  location_config.Display();

  TRPC_LOG_DEBUG("--------------------------------");
}

void ServerConnectorConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("timeout:" << timeout);
  TRPC_LOG_DEBUG("jointpoint:" << join_point);
  TRPC_LOG_DEBUG("protocol:" << protocol);
  TRPC_LOG_DEBUG("refresh_interval:" << refresh_interval);
  TRPC_LOG_DEBUG("addresses:");
  for (auto const& it : addresses) {
    TRPC_LOG_DEBUG(it);
  }
  TRPC_LOG_DEBUG("server_switch_interval:" << server_switch_interval);

  TRPC_LOG_DEBUG("--------------------------------");
}

void ServerMetricConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("name:" << name);
  TRPC_LOG_DEBUG("enable:" << enable);
  TRPC_LOG_DEBUG("metrics_name:" << metrics_name);

  TRPC_LOG_DEBUG("--------------------------------");
}

void GlobalConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  system_config.Display();
  api_config.Display();
  server_connector_config.Display();
  server_metric_config.Display();

  TRPC_LOG_DEBUG("--------------------------------");
}

void LocalCacheConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("type:" << type);
  TRPC_LOG_DEBUG("service_expire_time:" << service_expire_time);
  TRPC_LOG_DEBUG("service_refresh_interval:" << service_refresh_interval);
  TRPC_LOG_DEBUG("persist_dir:" << persist_dir);

  TRPC_LOG_DEBUG("--------------------------------");
}

void ErrorCountConfig::Display() const {
  TRPC_LOG_DEBUG("----------------ErrorCountConfig begin----------------");
  TRPC_LOG_DEBUG("continuousErrorThreshold:" << continuous_error_threshold);
  TRPC_LOG_DEBUG("sleepWindow:" << sleep_window);
  TRPC_LOG_DEBUG("requestCountAfterHalfOpen:" << request_count_after_halfopen);
  TRPC_LOG_DEBUG("successCountAfterHalfOpen:" << success_count_after_halfopen);
  TRPC_LOG_DEBUG("metricExpiredTime:" << metric_expired_time);
}

void ErrorRateConfig::Display() const {
  TRPC_LOG_DEBUG("----------------ErrorCountConfig begin----------------");
  TRPC_LOG_DEBUG("requestVolumeThreshold:" << request_volume_threshold);
  TRPC_LOG_DEBUG("errorRateThreshold:" << error_rate_threshold);
  TRPC_LOG_DEBUG("metricStatTimeWindow:" << metric_stat_time_window);
  TRPC_LOG_DEBUG("metricNumBuckets:" << metric_num_buckets);
  TRPC_LOG_DEBUG("sleepWindow:" << sleep_window);
  TRPC_LOG_DEBUG("requestCountAfterHalfOpen:" << request_count_after_halfopen);
  TRPC_LOG_DEBUG("successCountAfterHalfOpen:" << success_count_after_halfopen);
  TRPC_LOG_DEBUG("metricExpiredTime:" << metric_expired_time);
}

void CircuitBreakerPluginConfig::Display() const {
  TRPC_LOG_DEBUG("---------------CircuitBreakerPluginConfig begin-----------------");
  error_count_config.Display();
  error_rate_config.Display();
}

void SetCircuitBreakerConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");
  TRPC_LOG_DEBUG("enable: " << enable);
}

void CircuitBreakerConfig::Display() const {
  TRPC_LOG_DEBUG("---------------CircuitBreakerConfig begin-----------------");

  TRPC_LOG_DEBUG("enable: " << enable);
  TRPC_LOG_DEBUG("check_period:" << check_period);
  TRPC_LOG_DEBUG("chain:");
  for (auto const& it : chain) {
    TRPC_LOG_DEBUG(it);
  }
  plugin_config.Display();
  set_circuitbreaker_config.Display();
}

void OutlierDetectionConfig::Display() const {
  TRPC_LOG_DEBUG("---------------OutlierDetectionConfig begin-----------------");
  TRPC_LOG_DEBUG("enable:" << enable);
  TRPC_LOG_DEBUG("check_period:" << check_period);
}

void LoadBalancerConfig::Display() const {
  TRPC_LOG_DEBUG("---------------LoadBalancerConfig begin-----------------");
  TRPC_LOG_DEBUG("type:" << type);
  TRPC_LOG_DEBUG("enable_dynamic_weight:" << enable_dynamic_weight);
  TRPC_LOG_DEBUG("vnode_count:" << vnode_count);
  TRPC_LOG_DEBUG("compatible_golang:" << compatible_golang);
}

void NearbyBasedRouterConfig::Display() const {
  TRPC_LOG_DEBUG("---------------NearbyBasedRouterConfig begin-----------------");
  TRPC_LOG_DEBUG("match_level:" << match_level);
  TRPC_LOG_DEBUG("max_match_level:" << max_match_level);
  TRPC_LOG_DEBUG("strict_nearby:" << strict_nearby);
  TRPC_LOG_DEBUG("enable_degrade_by_unhealthy_percent:" << enable_degrade_by_unhealthy_percent);
  TRPC_LOG_DEBUG("unhealthy_percent_to_degrade:" << unhealthy_percent_to_degrade);
  TRPC_LOG_DEBUG("enable_recover_all:" << enable_recover_all);
}

void RouterPluginConfig::Display() const {
  TRPC_LOG_DEBUG("---------------RouterPluginConfig begin-----------------");
  nearby_based_router_config.Display();
}

void ServiceRouterConfig::Display() const {
  TRPC_LOG_DEBUG("---------------ServiceRouterConfig begin-----------------");
  TRPC_LOG_DEBUG("enable:" << enable);
  TRPC_LOG_DEBUG("percentOfMinInstances" << percent_of_min_instances);
  TRPC_LOG_DEBUG("chain:");
  for (auto const& it : chain) {
    TRPC_LOG_DEBUG(it);
  }
  router_plugin_config.Display();
}

void ServiceConsumerConfig::Display() const {
  TRPC_LOG_DEBUG("name:" << service_name);
  TRPC_LOG_DEBUG("namespace:" << service_namespace);
  if (set_circuit_breaker) {
    circuit_breaker_config.Display();
  }
  if (set_outlier_detection) {
    outlier_detect_config.Display();
  }
  if (set_load_balancer) {
    load_balancer_config.Display();
  }
  if (set_service_router) {
    service_router_config.Display();
  }
}

void ConsumerConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  local_cache_config.Display();

  load_balancer_config.Display();

  circuit_breaker_config.Display();

  outlier_detect_config.Display();

  service_router_config.Display();

  for (const auto& it : service_consumer_config) {
    it.Display();
  }

  TRPC_LOG_DEBUG("--------------------------------");
}

void DynamicWeightConfig::Display() const {
  TRPC_LOG_DEBUG("---------------DynamicWeightConfig begin-----------------");
  TRPC_LOG_DEBUG("open_dynamic_weight:" << open_dynamic_weight);
}

void SelectorConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  global_config.Display();
  consumer_config.Display();
  dynamic_weight_config.Display();

  TRPC_LOG_DEBUG("--------------------------------");
}

void RateLimiterConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("timeout:" << timeout);
  TRPC_LOG_DEBUG("updateCallResult:" << update_call_result);
  TRPC_LOG_DEBUG("mode:" << mode);
  cluster_config.Display();

  TRPC_LOG_DEBUG("--------------------------------");
}

void PolarisMeshNamingConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("name:" << name);
  registry_config.Display();
  selector_config.Display();
  ratelimiter_config.Display();
  TRPC_LOG_DEBUG("orig_selector_config:" << orig_selector_config);

  TRPC_LOG_DEBUG("--------------------------------");
}

}  // namespace trpc::naming
