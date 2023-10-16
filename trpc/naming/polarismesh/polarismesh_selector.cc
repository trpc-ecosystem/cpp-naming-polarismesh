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

#include <map>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

#include "google/protobuf/util/json_util.h"
#include "polaris/api/consumer_api.h"
#include "polaris/model/constants.h"
#include "polaris/plugin/service_router/set_division_router.h"

#include "trpc/codec/trpc/trpc.pb.h"
#include "trpc/naming/polarismesh/common.h"
#include "trpc/naming/polarismesh/config/polarismesh_naming_conf.h"
#include "trpc/naming/polarismesh/trpc_share_context.h"
#include "trpc/naming/selector_factory.h"
#include "trpc/util/log/logging.h"
#include "trpc/util/string_helper.h"
#include "trpc/util/string_util.h"

namespace trpc {

namespace naming::polarismesh {

uint32_t g_polarismesh_selector_plugin_id;

uint32_t GetPolarisMeshSelectorPluginID() {
  return g_polarismesh_selector_plugin_id;
}

}  // namespace naming::polarismesh


// Set the transparent information of the Selector-META-prefix to the polaris Routing rule
void SetTransSelectorMeta(const ClientContextPtr& context, std::map<std::string, std::string>* metadata) {
  static constexpr char meta_prefix[] = "selector-meta-";
  const auto& trans_info = context->GetPbReqTransInfo();
  for (auto& item : trans_info) {
    if (StartsWith(item.first, meta_prefix)) {
      std::string key = item.first.substr(sizeof(meta_prefix) - 1);
      (*metadata)[key] = item.second;
      TRPC_FMT_DEBUG("Set selector metadata, key:{}, value:{}", key, item.second);
    }
  }
}

int PolarisMeshSelector::Init() noexcept {
  if (init_) {
    TRPC_FMT_DEBUG("Already init");
    return 0;
  }

  naming::polarismesh::g_polarismesh_selector_plugin_id = naming::polarismesh::GetPolarisMeshSelectorPluginID();

  if (plugin_config_.name.empty()) {
    trpc::naming::PolarisMeshNamingConfig config;
    SetPolarisMeshSelectorConf(config);
    plugin_config_ = config;
  }

  enable_set_circuitbreaker_ =
      plugin_config_.selector_config.consumer_config.circuit_breaker_config.set_circuitbreaker_config.enable;
  timeout_ = plugin_config_.selector_config.global_config.server_connector_config.timeout;
  enable_polarismesh_trans_meta_ = plugin_config_.selector_config.consumer_config.enable_trans_meta;

  if (trpc::TrpcShareContext::GetInstance()->Init(plugin_config_) != 0) {
    return -1;
  }
  polarismesh_context_ = trpc::TrpcShareContext::GetInstance()->GetPolarisContext();
  consumer_api_ = std::unique_ptr<polaris::ConsumerApi>(polaris::ConsumerApi::Create(polarismesh_context_.get()));
//  polaris::GetLogger()->SetLogLevel(polaris::kDebugLogLevel);
//  auto polarismesh_context_ = polaris::ConsumerApi::CreateWithDefaultFile();
  //consumer_api_ = std::unique_ptr<polaris::ConsumerApi>(polarismesh_context_);
  if (!consumer_api_) {
    TRPC_FMT_ERROR("Create ConsumerApi failed");
    return -1;
  }

  // Add the default framework and return code white list
  circuitbreak_whitelist_.Writer().clear();
  circuitbreak_whitelist_.Writer().insert(TrpcRetCode::TRPC_SERVER_OVERLOAD_ERR);
  circuitbreak_whitelist_.Writer().insert(TrpcRetCode::TRPC_SERVER_LIMITED_ERR);
  circuitbreak_whitelist_.Swap();

  init_ = true;
  return 0;
}

void PolarisMeshSelector::Destroy() noexcept {
  if (!init_) {
    TRPC_FMT_DEBUG("No init yet");
    return;
  }

  consumer_api_ = nullptr;
  polarismesh_context_ = nullptr;
  trpc::TrpcShareContext::GetInstance()->Destroy();
  init_ = false;
}

// Obtain the specific implementation of the service node
// from the SDK API interface to select a single node or backup node
int PolarisMeshSelector::SelectImpl(const SelectorInfo* info, polaris::InstancesResponse*& polarismesh_response_info) {
  // The main system of service key
  polaris::ServiceInfo source_service_info;
  GetSourceServiceKey(info->context, info->extend_select_info, source_service_info.service_key_);
  // The adjusted service key
  std::string service_namespace = source_service_info.service_key_.namespace_;
  polaris::ServiceKey service_key{GetNamespaceFromContextOrExtend(info->context, info->extend_select_info), info->name};
  polaris::GetOneInstanceRequest request(service_key);

  // For the polarismesh, the load balancing plugin name and load balancing strategy are an option
  if (info->load_balance_name.empty()) {
    request.SetLoadBalanceType(polaris::kLoadBalanceTypeDefaultConfig);
  } else {
    request.SetLoadBalanceType(info->load_balance_name);
  }
  // Check HASH
  auto& hash_key = info->context->GetHashKey();
  if (!hash_key.empty()) {
    request.SetHashString(hash_key);
    // Set up a copy indexy
    uint64_t replicate_index = trpc::util::Convert<uint64_t, std::string>(
                                  GetValueFromContextOrExtend(info->context, info->extend_select_info, "replicate_index"));
    request.SetReplicateIndex(replicate_index);
    // polarismesh bug, use SethashKey in the early stages
    uint64_t u64_hash_key = trpc::util::Convert<uint64_t, std::string>(
                                hash_key);
    request.SetHashKey(u64_hash_key);
  }

  // Set the main service information
  FillMetadataOfSourceServiceInfo(info, source_service_info);

  // Set Canary Information
  auto cannary = GetValueFromContextOrExtend(info->context, info->extend_select_info, "canary_label");
  if (!cannary.empty()) {
    request.SetCanary(cannary);
  }

  request.SetSourceService(source_service_info);
  // Set timeout time
  request.SetTimeout(timeout_);

  // Fill in metadata
  auto meta = naming::polarismesh::GetFilterMetadataOfNaming(info->context,
                                                             PolarisMetadataType::kPolarisDstMetaRouteLable);
  if (meta != nullptr) {
    request.SetMetadata(*(meta.get()));
  }

  // If it is a backup strategy, you need to set the number of Backup nodes
  if (info->policy == SelectorPolicy::MULTIPLE && info->select_num > 1) {
    // SDK SETBACKUPINSTANCENUM method logic, the number does not include the first node
    request.SetBackupInstanceNum(info->select_num - 1);
  }

  // When selecting a routing, do not consider whether to include a health or melting node
  polaris::ReturnCode ret = consumer_api_->GetOneInstance(request, polarismesh_response_info);
  if (ret != polaris::ReturnCode::kReturnOk) {
    TRPC_FMT_ERROR("GetOneInstance failed, sdk returnCode:{}, service_name:{}, service_namespace:{}",
                   static_cast<int32_t>(ret), service_key.name_, service_key.namespace_);
    return -1;
  }
  return 0;
}

// Get the routing interface of a node that is transferred to a node
int PolarisMeshSelector::Select(const SelectorInfo* info, TrpcEndpointInfo* endpoint) {
  if (!init_) {
    TRPC_FMT_ERROR("No inited yet");
    return -1;
  }

  polaris::InstancesResponse* polarismesh_response_info;
  int ret = SelectImpl(info, polarismesh_response_info);
  if (ret != 0) {
    return -1;
  }

  std::vector<polaris::Instance>& instances = polarismesh_response_info->GetInstances();

  TRPC_ASSERT(instances.size() == 1 && "select result should return only one instance");
  if (info->is_from_workflow) {
    // From the workflow of the framework, the entire MetAdata of Instance
    ConvertPolarisInstance(instances[0], *endpoint, false);
    // SetInstanceId(polarismesh_response_info.GetId());
  } else {
    ConvertPolarisInstance(instances[0], *endpoint, true);
  }
  TRPC_FMT_DEBUG("Select result {}:{}, id:{}, service_name:{}, service_namespace:{}", endpoint->host, endpoint->port,
                 endpoint->id, info->name, GetValueFromContextOrExtend(info->context, info->extend_select_info, "namespace"));

  return 0;
}

// Asynchronous acquisition of a adjustable node interface
Future<TrpcEndpointInfo> PolarisMeshSelector::AsyncSelect(const SelectorInfo* info) {
  TrpcEndpointInfo endpoint;
  // Only blocked when the first call is called
  int ret = Select(info, &endpoint);
  if (ret != 0) {
    return MakeExceptionFuture<TrpcEndpointInfo>(CommonException("AsyncSelect error"));
  }

  return MakeReadyFuture<TrpcEndpointInfo>(std::move(endpoint));
}

// Obtain the interface of node routing information according to strategy: Support press SET, press IDC, press ALL, and
// at the backup condition.
int PolarisMeshSelector::SelectBatch(const SelectorInfo* info, std::vector<TrpcEndpointInfo>* endpoints) {
  if (!init_) {
    TRPC_FMT_ERROR("No init yet");
    return -1;
  }

  polaris::InstancesResponse* discovery_rsp = nullptr;

  if (info->policy == SelectorPolicy::MULTIPLE) {
    polaris::InstancesResponse* polarismesh_response_info;
    // Backup strategy (compatible with old version logic)
    int ret = SelectImpl(info, polarismesh_response_info);
    if (ret != 0) {
      return -1;
    }

    std::vector<polaris::Instance> instances = polarismesh_response_info->GetInstances();
    ConvertPolarisInstances(instances, *endpoints);

    return 0;
  }

  // The main system of service key
  polaris::ServiceKey source_service_key;
  GetSourceServiceKey(info->context, info->extend_select_info, source_service_key);
  // The adjusted service key
  std::string service_namespace = source_service_key.namespace_;
  polaris::ServiceKey service_key{service_namespace, info->name};

  polaris::GetInstancesRequest discovery_req = polaris::GetInstancesRequest(service_key);
  discovery_req.SetTimeout(timeout_);

  if (info->policy == SelectorPolicy::ALL) {
    // All nodes returned from the SDK interface are consistent with the polarismesh Console, including nodes with a
    // weight of 0 or isolation In the following code, it will remove nodes with isolation or 0 weights (compatible with
    // old version logic)
    polaris::ReturnCode ret = consumer_api_->GetAllInstances(discovery_req, discovery_rsp);
    if (ret != polaris::ReturnCode::kReturnOk) {
      TRPC_FMT_ERROR("GetAllInstances failed, sdk returnCode:{}, service_name:{}, service_namespace:{}",
                     static_cast<int32_t>(ret), service_key.name_, service_key.namespace_);
      if (discovery_rsp != nullptr) {
        delete discovery_rsp;
      }
      return -1;
    }
  } else {
    // Routing selection
    // Setting whether to include unhealthy or fuse nodes
    if (GetValueFromContextOrExtend(info->context, info->extend_select_info, "include_unhealthy") == "true") {
      discovery_req.SetIncludeUnhealthyInstances(true);
      discovery_req.SetIncludeCircuitBreakInstances(true);
    }

    // Set the main service information
    polaris::ServiceInfo source_service_info;
    source_service_info.service_key_ = source_service_key;
    FillMetadataOfSourceServiceInfo(info, source_service_info);
    // Set Canary Information
    auto cannary = GetValueFromContextOrExtend(info->context, info->extend_select_info, "canary_label");
    if (!cannary.empty()) {
      discovery_req.SetCanary(cannary);
    }
    discovery_req.SetSourceService(source_service_info);

    // Fill in metadata
    auto meta = naming::polarismesh::GetFilterMetadataOfNaming(info->context,
                                                               PolarisMetadataType::kPolarisDstMetaRouteLable);
    if (meta) {
      discovery_req.SetMetadata(*(meta.get()));
    }

    polaris::ReturnCode ret = consumer_api_->GetInstances(discovery_req, discovery_rsp);
    if (ret != polaris::ReturnCode::kReturnOk) {
      TRPC_FMT_ERROR("GetInstances failed, sdk returnCode:{}, service_name:{}, service_namespace:{}",
                     static_cast<int32_t>(ret), service_key.name_, service_key.namespace_);
      if (discovery_rsp != nullptr) {
        delete discovery_rsp;
      }
      return -1;
    }
  }

  TRPC_ASSERT(discovery_rsp != nullptr && "GetInstances or GetAllInstances success, but InstancesResponse is nullptr");

  std::vector<polaris::Instance>& instances = discovery_rsp->GetInstances();
  if (info->policy == SelectorPolicy::ALL) {
    // (Compatible with the old logic of the framework) When checking all nodes, exclude nodes with weight 0 or
    // isolation
    ConvertInstancesNoIsolated(instances, *endpoints);
  } else {
    ConvertPolarisInstances(instances, *endpoints);
  }

  delete discovery_rsp;
  return 0;
}

Future<std::vector<TrpcEndpointInfo>> PolarisMeshSelector::AsyncSelectBatch(const SelectorInfo* info) {
  std::vector<TrpcEndpointInfo> endpoints;
  // Only blocked when the first call is called
  int ret = SelectBatch(info, &endpoints);
  if (ret != 0) {
    return MakeExceptionFuture<std::vector<TrpcEndpointInfo>>(CommonException("AsyncSelectBatch error"));
  }

  return MakeReadyFuture<std::vector<TrpcEndpointInfo>>(std::move(endpoints));
}

// Report interface on the result
int PolarisMeshSelector::ReportInvokeResult(const InvokeResult* result) {
  if (!init_ || result == nullptr) {
    TRPC_FMT_ERROR("Invalid parameter: invoke result is invalid");
    return -1;
  }

  polaris::ServiceCallResult result_req;
  polaris::ServiceKey source_service_key;
  GetSourceServiceKey(result->context, nullptr, source_service_key);

  result_req.SetSource(source_service_key);
  result_req.SetServiceName(result->name);
  result_req.SetServiceNamespace(source_service_key.namespace_);
  result_req.SetInstanceHostAndPort(result->context->GetIp(), result->context->GetPort());

  // Set RetStatus (frame error code)
  result_req.SetRetStatus(FrameworkRetToPolarisRet(circuitbreak_whitelist_, result->framework_result));
  // Call_ret_code is a customized return value for users, for statistical reporting
  result_req.SetRetCode(result->interface_result);
  result_req.SetDelay(result->cost_time);
  // load balancing algorithm Info
  uint64_t locality_aware_info = trpc::util::Convert<uint64_t, std::string>(
                                    GetValueFromContextOrExtend(result->context, nullptr, "locality_aware_info"));
  if (locality_aware_info != 0) {
    result_req.SetLocalityAwareInfo(locality_aware_info);
  }

  auto circuit_breaker_lables = naming::polarismesh::GetFilterMetadataOfNaming(
      result->context, PolarisMetadataType::kPolarisCircuitBreakLable);
  if (circuit_breaker_lables) {
    result_req.SetLabels(*(circuit_breaker_lables.get()));
  }

  int ret = consumer_api_->UpdateServiceCallResult(result_req);
  if (ret != polaris::ReturnCode::kReturnOk) {
    TRPC_FMT_ERROR("UpdateServiceCallResult failed, sdk returnCode:{}, service_name:{}, service_namespace:{}",
                   static_cast<int32_t>(ret), result->name, source_service_key.namespace_);
    return -1;
  }

  return 0;
}

bool PolarisMeshSelector::SetCircuitBreakWhiteList(const std::vector<int>& framework_retcodes) {
  auto& writer = circuitbreak_whitelist_.Writer();
  writer.clear();
  writer.insert(framework_retcodes.begin(), framework_retcodes.end());
  circuitbreak_whitelist_.Swap();
  return true;
}

void PolarisMeshSelector::FillMetadataOfSourceServiceInfo(const SelectorInfo* info,
                                                      polaris::ServiceInfo& source_service_info) {
  const auto& context = info->context;
  auto& metadata = source_service_info.metadata_;

  auto filter_meta = naming::polarismesh::GetFilterMetadataOfNaming(info->context,
                                                                    PolarisMetadataType::kPolarisRuleRouteLable);
  if (filter_meta) {
    metadata = *(filter_meta.get());
  }

  // Set the ENV of the main party as the ENV in the frame configuration
  metadata["env"] = TrpcConfig::GetInstance()->GetGlobalConfig().env_name;

  // To solve the problem that the requesting field cannot be passed to the polarismesh for Meta matching.It agrees that
  // the transparent field of the prefix of the "Selector-Meta-'prefix, remove the prefix and fill in Meta, and match
  // the polarismesh
  if (enable_polarismesh_trans_meta_) {
    SetTransSelectorMeta(context, &metadata);
    TRPC_FMT_DEBUG("Enable trans selector meta, service_name:{}", info->name);
  }

  // Set the main information of the main party
  metadata[polaris::SetDivisionServiceRouter::enable_set_force] =
                                                     GetValueFromContextOrExtend(info->context, info->extend_select_info, "enable_set_force");
  TRPC_FMT_DEBUG("Enable set force, service_name:{}", info->name);

  if (!GetValueFromContextOrExtend(info->context, info->extend_select_info, "callee_set_name").empty()) {
    metadata[polaris::constants::kRouterRequestSetNameKey] =
        GetValueFromContextOrExtend(info->context, info->extend_select_info, "callee_set_name");
    TRPC_FMT_DEBUG("Add source setname to metadata, CalleeSetName:{} service_name:{}",
                        GetValueFromContextOrExtend(info->context, info->extend_select_info, "callee_set_name"), info->name);
  }
}

void PolarisMeshSelector::GetSourceServiceKey(const ClientContextPtr& client_context_ptr,
                                          const std::any* extend_select_info, polaris::ServiceKey& service_key) {
  service_key.name_ = client_context_ptr->GetCallerName();
  auto& global_namespace = TrpcConfig::GetInstance()->GetGlobalConfig().env_namespace;
  if (!global_namespace.empty()) {
    service_key.namespace_ = global_namespace;
  } else {
    service_key.namespace_ = GetNamespaceFromContextOrExtend(client_context_ptr, extend_select_info);
  }

  return ;
}

std::string PolarisMeshSelector::ParseExtendSelectInfo(const std::any* extend_select_info, const std::string& field_name) {
  if (extend_select_info == nullptr) {
    return "";
  }

  try {
    // Attempt to convert std::any to std::unordered_map<std::string, std::string>
    std::unordered_map<std::string, std::string> extend_select_info_map =
        std::any_cast<std::unordered_map<std::string, std::string>>(*extend_select_info);

    // Look for the required field value
    auto it = extend_select_info_map.find(field_name);
    if (it != extend_select_info_map.end()) {
      return it->second;
    }
  } catch (const std::bad_any_cast& e) {
    TRPC_FMT_ERROR("Failed to cast extend_select_info to std::unordered_map<std::string, std::string>: {}",
                   e.what());
  }

  // If the field is not found or the conversion fails, return an empty string
  return "";
}

std::string PolarisMeshSelector::GetValueFromContextOrExtend(const ClientContextPtr& context,
                                                          const std::any* extend_select_info, const std::string& field_name) {
  std::string value = naming::polarismesh::GetSelectorExtendInfo(context, field_name);
  if (value.empty()) {
    value = ParseExtendSelectInfo(extend_select_info, field_name);
  }
  return value;
}

std::string PolarisMeshSelector::GetNamespaceFromContextOrExtend(const ClientContextPtr& context,
                                                                 const std::any* extend_select_info) {
  std::string value = naming::polarismesh::GetSelectorExtendInfo(context, "namespace");

  if (value.empty()) {
    value = ParseExtendSelectInfo(extend_select_info, "namespace");
    if (value.empty()) {
      // If the value is still empty, try to get the namespace from ServiceProxyOption
      const ServiceProxyOption* service_proxy_option = context->GetServiceProxyOption();
      if (service_proxy_option != nullptr) {
        value = service_proxy_option->name_space;
      }
    }
    // If the namespace is obtained from ParseExtendSelectInfo or ServiceProxyOption, set it in the context
    if (!value.empty()) {
      naming::polarismesh::SetSelectorExtendInfo(context, std::make_pair("namespace", value));
    }
  }

  return value;
}

}  // namespace trpc
