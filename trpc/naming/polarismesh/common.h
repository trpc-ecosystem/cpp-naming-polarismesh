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
#include <memory>
#include <string>
#include <vector>

#include "polaris/model/model_impl.h"

#include "readers_writer_data.h"
#include "trpc/client/client_context.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/status.h"
#include "trpc/naming/common/common_defs.h"
#include "trpc/naming/polarismesh/config/polaris_naming_conf.h"

namespace trpc {

/// @brief polarismesh SDK version
static const char kPolarisPluginName[] = "polarismesh";
static const char kPolarisSDKVersion[] = "1.1.1";

// Polaris-related metadata types
enum PolarisMetadataType {
  // Polaris rule route label type
  kPolarisRuleRouteLable,
  // Polaris metadata route label type
  kPolarisDstMetaRouteLable,
  // Polaris circuit breaker label type
  kPolarisCircuitBreakLable,
  // Polaris subset label used for do circuit break by set
  kPolarisSubsetLabel,
  // Number of metadata type
  kPolarisTypeNum,
};

struct PolarisExtendSelectInfo {
  std::string name_space;
  std::string callee_set_name;
  std::string canary_label;
  bool enable_set_force{false};
  bool disable_servicerouter{false};
  uint64_t locality_aware_info{0};
  uint32_t replicate_index{0};
  std::map<std::string, std::string> metadata[PolarisMetadataType::kPolarisTypeNum];
  bool include_unhealthy{false};
};

namespace object_pool {

template <>
struct ObjectPoolTraits<PolarisExtendSelectInfo> {
#if defined(TRPC_DISABLED_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kDisabled;
#elif defined(TRPC_SHARED_NOTHING_OBJECTPOOL)
  static constexpr auto kType = ObjectPoolType::kSharedNothing;
#else
  static constexpr auto kType = ObjectPoolType::kGlobal;
#endif
};

}  // namespace object_pool

/// @brief Convert polarismesh Service Institutional Structure to a Service Example Structure defined by framework
/// definition
/// @param[in] instance The service instance structure defined by the polarismesh SDK
/// @param[out] endpoint Service institutional structure defined by the framework
/// @param[in] need_meta Do you need to fill in META information
inline void ConvertPolarisInstance(const polaris::Instance& instance, TrpcEndpointInfo& endpoint,
                                   bool need_meta = true) {
  endpoint.host = instance.GetHost();
  endpoint.port = instance.GetPort();
  endpoint.is_ipv6 = instance.IsIpv6();
  endpoint.status = instance.isHealthy();
  endpoint.weight = instance.GetWeight();
  endpoint.id = instance.GetLocalId();
  // Fill in metadata information
  if (need_meta) {
    endpoint.meta = instance.GetMetadata();
    endpoint.meta["instance_id"] = instance.GetId();
  }
}

/// @brief Swap the batch polarismesh Service Institutional Structure into a Service Example Structure defined by the
/// framework
/// @param[in] instances Batch polarismesh SDK definition service instance structure
/// @param[out] endpoints Batch framework definition service instance structure
inline void ConvertPolarisInstances(const std::vector<polaris::Instance>& instances,
                                    std::vector<TrpcEndpointInfo>& endpoints) {
  endpoints.reserve(instances.size());
  for (const auto& item : instances) {
    TrpcEndpointInfo result;
    ConvertPolarisInstance(item, result);
    endpoints.emplace_back(result);
  }
}

/// @brief (Compatible framework Old logic) Remove nodes with weight 0 or isolation during batch conversion
///        Swap the batch polarismesh Service Institutional Structure into a Service Example Structure defined by the
///        framework
/// @param[in] instances Batch polarismesh SDK definition service instance structure
/// @param[out] endpoints Batch framework definition service instance structure
inline void ConvertInstancesNoIsolated(const std::vector<polaris::Instance>& instances,
                                                  std::vector<TrpcEndpointInfo>& endpoints) {
  endpoints.reserve(instances.size());
  for (const auto& item : instances) {
    if (item.isIsolate() || item.GetWeight() == 0) {
      continue;
    }
    TrpcEndpointInfo result;
    ConvertPolarisInstance(item, result);
    endpoints.emplace_back(result);
  }
}

/// @brief Obtain the key corresponding to the key from Metadata
/// @param[in] metadata data set
/// @param[in] key key
/// @param[out] value value
/// @return INT successfully returns 0, fails to return -1
inline int GetValueFromMetadata(const std::map<std::string, std::string>& metadata, const std::string& key,
                                std::string& value) {
  auto iter = metadata.find(key);
  if (iter == metadata.end()) {
    return -1;
  }

  value = iter->second;
  return 0;
}

/// @brief Obtaining the String type value corresponding to the key from Metadata, without the default value
/// @param metadata data set
/// @param key key
/// @param default_value Defaults
/// @return string The value corresponding to the key
inline std::string GetStringFromMetadata(const std::map<std::string, std::string>& metadata, const std::string& key,
                                         const std::string& default_value) {
  std::string value;
  if (GetValueFromMetadata(metadata, key, value) != 0) {
    return default_value;
  }

  return value;
}

/// @brief Obtain the INT type value corresponding to the key from Metadata, without the default value
/// @param metadata data set
/// @param key key
/// @param default_value Defaults
/// @return int The value corresponding to the key
inline int GetIntFromMetadata(const std::map<std::string, std::string>& metadata, const std::string& key,
                              const int default_value) {
  std::string value;
  if (GetValueFromMetadata(metadata, key, value) != 0) {
    return default_value;
  }

  return std::atoi(value.c_str());
}

/// @brief polarismesh Service Example Registration Information Structure
struct PolarisRegistryInfo {
  /// The flowing number is used to track the user's request, optional, default 0
  uint64_t flow_id;
  /// Service name, optional, default empty, but in the case of empty, if the SID information is empty, the error will
  /// be reported
  std::string service_name;
  /// Name space, optional, default space
  std::string service_namespace;
  /// Service access token
  std::string service_token;
  /// Service monitoring host, support IPv6 address
  std::string host;
  /// Service Surveillance listening to PORT
  int port;
  /// The maximum timeout information of this inquiry, optional, default to obtain the global overtime configuration by
  /// default
  uint64_t timeout;
  /// Service instance ID
  std::string instance_id;
  /// Service protocol, optional
  std::string protocol;
  /// Service weight, default 100, range 0-1000
  int weight;
  /// Example priority, the default is 0, the smaller the value, the higher the priority, the higher the priority
  int priority;
  /// Example provides service version number
  std::string version;
  /// User customized metadata information
  std::map<std::string, std::string> metadata;
  /// Whether to turn on a health check, 0 will not be opened, 1 open, silent 0, start a health check will start the
  /// backbone time -time task to report the heartbeat
  bool enable_health_check;
  /// Monitoring and Inspection Type: 0 is a heartbeat.
  int health_check_type;
  /// TTL timeout time, unit: second
  int ttl;
};

/// @brief transformTheServiceEntityOfTheServiceEntryOfTheUpperFrameOfTheFrame
/// @param[in] registry_info theServiceEntityRegistrationInformationBodyIntroducedInTheUpperLayerOfTheFramework
/// @param[out] polaris_info polarismeshServiceExampleRegistrationInformationStructure
inline void ConvertToPolarisRegistryInfo(const RegistryInfo& registry_info, PolarisRegistryInfo& polaris_info) {
  polaris_info.service_name = registry_info.name;
  polaris_info.host = registry_info.host;
  polaris_info.port = registry_info.port;
  auto& metadata = registry_info.meta;

  for (auto& var : metadata) {
    const std::string& key = var.first;
    if (!key.compare("token") || !key.compare("instance_id") || !key.compare("protocol") || !key.compare("weight") ||
        !key.compare("priority") || !key.compare("version") || !key.compare("enable_health_check") ||
        !key.compare("health_check_type") || !key.compare("ttl") || !key.compare("namespace")) {
      continue;
    }

    polaris_info.metadata[key] = var.second;
  }

  // thisMethodFocusesOnTheConversionOfIpportAndMetadataInformationNamespaceHasNotBeenConvertedThere
  polaris_info.service_token = GetStringFromMetadata(metadata, "token", "");
  polaris_info.instance_id = GetStringFromMetadata(metadata, "instance_id", "");
  polaris_info.protocol = GetStringFromMetadata(metadata, "protocol", "");
  polaris_info.weight = GetIntFromMetadata(metadata, "weight", 100);
  polaris_info.priority = GetIntFromMetadata(metadata, "priority", 0);
  polaris_info.version = GetStringFromMetadata(metadata, "version", "");
  polaris_info.enable_health_check = GetIntFromMetadata(metadata, "enable_health_check", 0);
  polaris_info.health_check_type = GetIntFromMetadata(metadata, "health_check_type", 0);
  polaris_info.ttl = GetIntFromMetadata(metadata, "ttl", 0);
}

/// @brief Is it the same to compare the serviceKey
/// @return BOOL TRUE indicates that the two are consistent, and false means that the two are different
inline bool ServiceKeyEqual(const polaris::ServiceKey& service_key1, const polaris::ServiceKey& service_key2) {
  if (service_key1.namespace_ == service_key2.namespace_ && service_key1.name_ == service_key2.name_) {
    return true;
  }

  return false;
}

/// @brief Integrate all information in config into config.orig_selector_config
///        Follow -up can construct the Context of the Arctic SDK through Orig_selector_config
/// @param config polarismesh plug -in configuration
void SetPolarisSelectorConf(trpc::naming::PolarisNamingConfig& config);

/// @brief The framework error code with the fuse of the whitening list is reported to the error code conversion on the
/// fuse of the polarismesh
/// @param circuitbreak_whitelist Breakfast list
/// @param framework_ret Framework error code
/// @return polarismesh::CallRetStatus polarismesh melting error code
polaris::CallRetStatus FrameworkRetToPolarisRet(ReadersWriterData<std::set<int>>& whitelist, int framework_ret);

}  // namespace trpc
