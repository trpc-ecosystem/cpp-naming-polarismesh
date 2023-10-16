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

#include <any>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "polaris/api/consumer_api.h"
#include "polaris/consumer.h"
#include "polaris/context.h"

#include "readers_writer_data.h"
#include "trpc/naming/common/common_defs.h"
#include "trpc/naming/polarismesh/common.h"
#include "trpc/naming/selector.h"

namespace trpc {

namespace naming::polarismesh {

/// @brief GetPolarisMeshSelectorPluginID() is a getter function to access the global variable
/// @return The PluginID of the PolarisMeshSelector instance as uint32_t.
uint32_t GetPolarisMeshSelectorPluginID();

/// @brief Sets selector-related extend properties in the context's filter data
/// This function allows users to set multiple key-value pairs related to the selector.
/// The following properties can be set using this function:
/// - namespace
/// - callee_set_name
/// - canary_label
/// - enable_set_force (boolean)
/// - disable_servicerouter (boolean)
/// - locality_aware_info (uint64_t)
/// - replicate_index (uint32_t)
/// - metadata (std::map<std::string, std::string>)
/// - include_unhealthy (boolean)
/// @param context Client/Server context to store the filter data, can be either serverContext or clientContext.
/// @param key_value_pairs A variadic list of key-value pairs to set in the context's filter data.
template <typename T, typename... Args>
void SetSelectorExtendInfo(T& context, Args&&... key_value_pairs) {
  auto* data_map = context->template GetFilterData<std::unordered_map<std::string,
                                                                      std::string>>(GetPolarisMeshSelectorPluginID());
  if (!data_map) {
    std::unordered_map<std::string, std::string> new_data_map;
    (new_data_map.emplace(std::forward<Args>(key_value_pairs)), ...);
    context->SetFilterData(GetPolarisMeshSelectorPluginID(), std::move(new_data_map));
  } else {
    (data_map->emplace(std::forward<Args>(key_value_pairs)), ...);
  }
}

/// @brief Gets the value of a selector-related property from the context's filter data
/// This function allows users to get the value of a specific key related to the selector.
/// The following properties can be retrieved using this function:
/// - namespace
/// - callee_set_name
/// - canary_label
/// - enable_set_force (boolean)
/// - disable_servicerouter (boolean)
/// - locality_aware_info (uint64_t)
/// - replicate_index (uint32_t)
/// - metadata (std::map<std::string, std::string>)
/// - include_unhealthy (boolean)
/// @param context Client/Server context to retrieve the filter data from, can be either serverContext or clientContext.
/// @param key The key of the property to retrieve.
/// @return The value of the specified property if found, or an empty string if not found.
template <typename T>
std::string GetSelectorExtendInfo(T& context, const std::string& key) {
  auto* data_map = context->template GetFilterData<std::unordered_map<std::string,
                                                                      std::string>>(GetPolarisMeshSelectorPluginID());
  if (data_map) {
    auto it = data_map->find(key);
    if (it != data_map->end()) {
      return it->second;
    }
  }
  return "";
}

/// @brief Sets the metadata for the specified PolarisMetadataType in the context's filter data
/// @tparam T The context type, can be either serverContext or clientContext
/// @param context The context to store the filter data
/// @param type The PolarisMetadataType index to set the metadata for
/// @param metadata_map The metadata map to set in the context's filter data
template <typename T>
void SetFilterMetadataOfNaming(T& context, const std::map<std::string,
                                                          std::string>& metadata_map,  PolarisMetadataType type) {
  const std::string key = "metadata_" + std::to_string(static_cast<int>(type));

  auto* data_map = context->template GetFilterData<std::unordered_map<std::string,
                                                                      std::string>>(GetPolarisMeshSelectorPluginID());
  if (!data_map) {
    std::unordered_map<std::string, std::string> new_data_map;

    rapidjson::Document json_metadata(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& allocator = json_metadata.GetAllocator();
    for (const auto& m : metadata_map) {
      json_metadata.AddMember(rapidjson::StringRef(m.first.c_str()), rapidjson::StringRef(m.second.c_str()), allocator);
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json_metadata.Accept(writer);

    new_data_map.emplace(key, buffer.GetString());
    context->SetFilterData(GetPolarisMeshSelectorPluginID(), std::move(new_data_map));
  } else {
    rapidjson::Document json_metadata(rapidjson::kObjectType);
    rapidjson::Document::AllocatorType& allocator = json_metadata.GetAllocator();
    for (const auto& m : metadata_map) {
      json_metadata.AddMember(rapidjson::StringRef(m.first.c_str()), rapidjson::StringRef(m.second.c_str()), allocator);
    }
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    json_metadata.Accept(writer);

    data_map->emplace(key, buffer.GetString());
  }
}

/// @brief Retrieves the metadata stored in the context
/// @tparam T The context type, can be either serverContext or clientContext
/// @param context The context from which to retrieve the metadata
/// @return A map containing the metadata, or an empty map if the metadata is not found
template <typename T>
std::unique_ptr<std::map<std::string, std::string>> GetFilterMetadataOfNaming(T& context, PolarisMetadataType type) {
  const std::string key = "metadata_" + std::to_string(static_cast<int>(type));

  auto* data_map = context->template GetFilterData<std::unordered_map<std::string,
                                                                      std::string>>(GetPolarisMeshSelectorPluginID());
  if (data_map) {
    auto it = data_map->find(key);
    if (it != data_map->end()) {
      rapidjson::Document json_metadata;
      json_metadata.Parse(it->second.c_str());
      auto metadata_map_ptr = std::make_unique<std::map<std::string, std::string>>();  // Create a new unique_ptr
      for (auto& m : json_metadata.GetObject()) {
        metadata_map_ptr->emplace(m.name.GetString(), m.value.GetString());
      }
      return metadata_map_ptr->size() > 0 ? std::move(metadata_map_ptr) : nullptr;
    }
  }

  return nullptr;
}

}  // namespace naming::polarismesh

/// @brief polarismesh service discovery plugin
class PolarisMeshSelector : public Selector {
 public:
  /// @brief The name of the plugin
  std::string Name() const override { return kPolarisPluginName; }

  /// @brief Plug -in version
  std::string Version() const override { return kPolarisSDKVersion; }

  /// @brief initialization
  int Init() noexcept override;

  /// @brief In the internal implementation of the plugin, it needs to be used when the thread needs to be created.
  /// You can use this interface uniformly
  void Start() noexcept override {}

  /// @brief When there is a thread in the internal implementation of the plugin, the interface of the stop thread
  /// needs to be implemented
  void Stop() noexcept override {}

  /// @brief Various resources to destroy specific plugin
  void Destroy() noexcept override;

  /// @brief Get the routing interface of a node that is transferred to a node
  int Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) override;

  /// @brief Asynchronous acquisition of a adjustable node interface
  Future<TrpcEndpointInfo> AsyncSelect(const SelectorInfo* info) override;

  /// @brief Obtain the interface of node routing information according to strategy
  int SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) override;

  /// @brief Asynchronously obtain the interface of node routing information according to strategy
  Future<std::vector<TrpcEndpointInfo>> AsyncSelectBatch(const SelectorInfo* info) override;

  /// @brief Report interface on the result
  int ReportInvokeResult(const InvokeResult* result) override;

  /// @brief Set up the route information interface of the service transfer
  int SetEndpoints(const RouterInfo* info) override { return 0; }

  /// @brief Set framework error codes melting white list
  bool SetCircuitBreakWhiteList(const std::vector<int>& framework_retcodes) override;
  
  /// @brief Setter function for plugin_config_
  void SetPluginConfig(const naming::PolarisMeshNamingConfig& config) {
    plugin_config_ = config;
  }

  /// @brief ServiceKey, the main tone
  /// @param[in] client_context_ptr Client context
  /// @param[out] service_key SERVICEKEY of the main tone
  void GetSourceServiceKey(const ClientContextPtr& client_context_ptr,
                           const std::any* extend_select_info, polaris::ServiceKey& service_key);

 private:
  // Get the specific implementation of the service node from the SDK GetoneInstance interface
  int SelectImpl(const SelectorInfo* info, polaris::InstancesResponse*& polarismesh_response_info);

  // Set the main service information
  void FillMetadataOfSourceServiceInfo(const SelectorInfo* info, polaris::ServiceInfo& source_service_info);

  // Parse extend_select_info and return the value of the required field;
  // if the field is not found, return an empty string.
  std::string ParseExtendSelectInfo(const std::any* extend_select_info, const std::string& field_name);

  // Tries to get the value for the given field_name from the context.
  // If the value is not found in the context, it tries to get it from the extend_select_info.
  std::string GetValueFromContextOrExtend(const ClientContextPtr& context,
                                          const std::any* extend_select_info, const std::string& field_name);

  // Tries to get the value for the given "namespace" from the context.
  std::string GetNamespaceFromContextOrExtend(const ClientContextPtr& context,
                                                               const std::any* extend_select_info);

 private:
  bool init_{false};

  // Whether to enable the SET melting function
  bool enable_set_circuitbreaker_;

  // The TRPC protocol transmission field is passed to the polarismesh for the switch used by Meta matching. The meta
  // format is "Selector-Meta-"
  bool enable_polarismesh_trans_meta_{false};

  // Framework error code fuse whitelist
  ReadersWriterData<std::set<int>> circuitbreak_whitelist_;

  // Service discovery timeout time, compatible with old configuration logic
  uint64_t timeout_;

  naming::PolarisMeshNamingConfig plugin_config_;
  std::shared_ptr<polaris::Context> polarismesh_context_{nullptr};
  std::unique_ptr<polaris::ConsumerApi> consumer_api_{nullptr};

  struct PolarisRuleRouteRaw {
    explicit PolarisRuleRouteRaw(polaris::ServiceData* data) : rule_route_data(data) {
      rule_route_data->IncrementRef();
    }
    ~PolarisRuleRouteRaw() {
      if (rule_route_data) {
        rule_route_data->DecrementRef();
        rule_route_data = nullptr;
      }
    }

    polaris::ServiceData* rule_route_data;
  };

  static thread_local std::unordered_map<std::string, PolarisRuleRouteRaw> source_route_rule_map_;
};

using PolarisMeshSelectorPtr = RefPtr<PolarisMeshSelector>;

}  // namespace trpc
