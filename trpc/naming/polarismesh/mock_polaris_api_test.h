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

#include <ftw.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "polaris/context/context_impl.h"
#include "polaris/defs.h"
#include "polaris/plugin.h"
#include "polaris/plugin/server_connector/server_connector.h"
#include "polaris/provider.h"
#include "utils/string_utils.h"
#include "utils/time_clock.h"
#include "utils/utils.h"
#include "v1/code.pb.h"
#include "yaml-cpp/yaml.h"

#include "trpc/naming/polarismesh/config/polaris_naming_conf.h"

namespace trpc {

static char kPolarisNamespaceTest[] = "Test";

// The switch of the polarismesh Test configuration item
struct PolarisNamingTestConfigSwitch {
  std::string bind_ip;
  std::string bind_if;  // Specified network card
  bool need_dstMetaRouter = false;
  bool need_circuitbreaker = false;
  bool need_ratelimiter = false;
};

inline std::string buildPolarisNamingConfig(const PolarisNamingTestConfigSwitch& default_switch) {
  trpc::naming::PolarisNamingConfig naming_config("polarismesh");

  // registry config
  naming_config.registry_config.heartbeat_interval = 1011;
  naming_config.registry_config.heartbeat_timeout = 2022;
  trpc::naming::ServiceConfig service_config;
  service_config.name = "test.service";
  service_config.namespace_ = kPolarisNamespaceTest;
  service_config.token = "8e8f342d3305443c983b2a420afce9d2";
  service_config.instance_id = "instance_0";
  naming_config.registry_config.services_config.push_back(service_config);

  // select config
  // select config - global - serverConnector
  naming_config.selector_config.global_config.server_connector_config.addresses = {"127.0.0.1:8090"};
  naming_config.selector_config.global_config.server_connector_config.timeout = 3033;
  naming_config.selector_config.global_config.server_connector_config.join_point = "default";
  naming_config.selector_config.global_config.server_connector_config.refresh_interval = 4044;
  // select config - global - system
  trpc::naming::ServiceClusterConfig discover_cluster_config;
  discover_cluster_config.service_name = "polarismesh.discover.pcg";
  discover_cluster_config.service_namespace = "Polaris";
  naming_config.selector_config.global_config.system_config.clusters_config["discoverCluster"] =
      discover_cluster_config;
  // select config - global - api
  if (!default_switch.bind_ip.empty()) {
    naming_config.selector_config.global_config.api_config.bind_ip = default_switch.bind_ip;
  }
  if (!default_switch.bind_if.empty()) {
    naming_config.selector_config.global_config.api_config.bind_if = default_switch.bind_if;
  }
  naming_config.selector_config.global_config.api_config.location_config.region = "";
  naming_config.selector_config.global_config.api_config.location_config.zone = "";
  naming_config.selector_config.global_config.api_config.location_config.campus = "";
  naming_config.selector_config.global_config.api_config.location_config.enable_update = true;
  // select config - consumer - localCache
  naming_config.selector_config.consumer_config.local_cache_config.type = "inmemory";
  naming_config.selector_config.consumer_config.local_cache_config.service_expire_time = "23h";
  naming_config.selector_config.consumer_config.local_cache_config.service_refresh_interval = 5055;
  naming_config.selector_config.consumer_config.local_cache_config.persist_dir =
      "/usr/local/trpc/data/polarismesh/backup";

  // select config - consumer - serviceRouter
  if (default_switch.need_dstMetaRouter) {
    naming_config.selector_config.consumer_config.service_router_config.chain.push_back("dstMetaRouter");
  }

  // select config - consumer - circuitBreaker
  if (default_switch.need_circuitbreaker) {
    // error count config
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_count_config
        .continuous_error_threshold = 5;
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_count_config.sleep_window =
        10000;
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_count_config
        .request_count_after_halfopen = 5;
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_count_config
        .success_count_after_halfopen = 3;
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_count_config
        .metric_expired_time = 1800000;

    // error rate config
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_rate_config
        .request_volume_threshold = 5;
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_rate_config
        .error_rate_threshold = 0.3;
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_rate_config
        .metric_stat_time_window = 30000;
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_rate_config
        .metric_num_buckets = 10;
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_rate_config.sleep_window =
        10000;
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_rate_config
        .request_count_after_halfopen = 5;
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_rate_config
        .success_count_after_halfopen = 3;
    naming_config.selector_config.consumer_config.circuit_breaker_config.plugin_config.error_rate_config
        .metric_expired_time = 1800000;
  }

  // select config - DynamicWeight
  naming_config.selector_config.dynamic_weight_config.open_dynamic_weight = false;

  // limiter config
  if (default_switch.need_ratelimiter) {
    naming_config.ratelimiter_config.timeout = 500;
    naming_config.ratelimiter_config.update_call_result = true;
    naming_config.ratelimiter_config.mode = "local";
    naming_config.ratelimiter_config.cluster_config.service_name = "polarismesh.metric.test";
    naming_config.ratelimiter_config.cluster_config.service_namespace = "Polaris";
  }

  YAML::Node naming_config_node(naming_config);
  std::stringstream strstream;
  strstream << naming_config_node;
  return strstream.str();
}

}  // namespace trpc

namespace polaris {

// TODO(hanqin): These Mock should be provided by Polaris SDK to business use
extern std::atomic<uint64_t> g_fake_system_time_ms;
extern std::atomic<uint64_t> g_fake_steady_time_ms;

class TestUtils {
 public:
  static void SetUpFakeTime() {
    g_fake_system_time_ms = Time::GetSystemTimeMs();
    g_fake_steady_time_ms = Time::GetCoarseSteadyTimeMs();
    Time::SetCustomTimeFunc(FakeSystemTime, FakeSteadyTime);
  }

  static void TearDownFakeTime() { Time::SetDefaultTimeFunc(); }

  static void FakeNowIncrement(uint64_t add_ms) {
    FakeSystemTimeInc(add_ms);
    FakeSteadyTimeInc(add_ms);
  }

  static void FakeSystemTimeInc(uint64_t add_ms) { g_fake_system_time_ms.fetch_add(add_ms, std::memory_order_relaxed); }

  static void FakeSteadyTimeInc(uint64_t add_ms) { g_fake_steady_time_ms.fetch_add(add_ms, std::memory_order_relaxed); }

 private:
  static uint64_t FakeSystemTime() { return g_fake_system_time_ms; }

  static uint64_t FakeSteadyTime() { return g_fake_steady_time_ms; }

 public:
  static int PickUnusedPort() {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = 0;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
      return -1;
    }

    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
      close(sock);
      return -1;
    }

    socklen_t addr_len = sizeof(addr);
    if (getsockname(sock, reinterpret_cast<struct sockaddr*>(&addr), &addr_len) != 0 || addr_len > sizeof(addr)) {
      close(sock);
      return -1;
    }
    close(sock);
    return ntohs(addr.sin_port);
  }

  static bool CreateTempFile(std::string& file) {
    char temp_file[] = "/tmp/polaris_test_XXXXXX";
    int fd = mkstemp(temp_file);
    if (fd < 0) return false;
    close(fd);
    file = temp_file;
    return true;
  }

  static bool CreateTempFileWithContent(std::string& file, const std::string& content) {
    char temp_file[] = "/tmp/polaris_test_XXXXXX";
    int fd = mkstemp(temp_file);
    if (fd < 0) return false;
    std::size_t len = write(fd, content.c_str(), content.size());
    if (len != content.size()) return false;
    close(fd);
    file = temp_file;
    return true;
  }

  static bool CreateTempDir(std::string& dir) {
    char temp_dir[] = "/tmp/polaris_test_XXXXXX";
    char* dir_name = mkdtemp(temp_dir);
    if (dir_name == NULL) return false;
    dir = temp_dir;
    return true;
  }

  static bool RemoveDir(const std::string& dir) { return nftw(dir.c_str(), RemoveFile, 10, FTW_DEPTH | FTW_PHYS) == 0; }

 private:
  static int RemoveFile(const char* path, const struct stat* /*sbuf*/, int /*type*/, struct FTW* /*ftwb*/) {
    return remove(path);
  }
};

class MockServerConnector : public ServerConnector {
 public:
  MockServerConnector() : saved_handler_(NULL) {}

  MOCK_METHOD2(Init, ReturnCode(Config* config, Context* context));

  MOCK_METHOD5(RegisterEventHandler,
               ReturnCode(const ServiceKey& service_key, ServiceDataType data_type, uint64_t sync_interval,
                          const std::string& disk_revision, ServiceEventHandler* handler));

  MOCK_METHOD2(DeregisterEventHandler, ReturnCode(const ServiceKey& service_key, ServiceDataType data_type));

  MOCK_METHOD3(RegisterInstance,
               ReturnCode(const InstanceRegisterRequest& req, uint64_t timeout_ms, std::string& instance_id));

  MOCK_METHOD2(DeregisterInstance, ReturnCode(const InstanceDeregisterRequest& req, uint64_t timeout_ms));

  MOCK_METHOD2(InstanceHeartbeat, ReturnCode(const InstanceHeartbeatRequest& req, uint64_t timeout_ms));

  MOCK_METHOD3(AsyncInstanceHeartbeat,
               ReturnCode(const InstanceHeartbeatRequest& req, uint64_t timeout_ms, ProviderCallback* callback));

  MOCK_METHOD3(AsyncReportClient, ReturnCode(const std::string& host, uint64_t timeout_ms, PolarisCallback callback));

  void SaveHandler(const ServiceKey& /*service_key*/, ServiceDataType /*data_type*/, uint64_t /*sync_interval*/,
                   const std::string& /*disk_revision*/, ServiceEventHandler* handler) {
    ASSERT_TRUE(handler != NULL);
    saved_handler_ = handler;
  }
  ServiceEventHandler* saved_handler_;

  void DeleteHandler(const ServiceKey& service_key, ServiceDataType data_type) {
    ASSERT_TRUE(saved_handler_ != NULL);
    saved_handler_->OnEventUpdate(service_key, data_type, NULL);
    delete saved_handler_;
    saved_handler_ = NULL;
  }
};

struct EventHandlerData {
  ServiceKey service_key_;
  ServiceDataType data_type_;
  ServiceData* service_data_;
  ServiceEventHandler* handler_;
};

class MockServerConnectorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    server_connector_ = new MockServerConnector();
    server_connector_plugin_name_ = "mock";
    ReturnCode ret = RegisterPlugin(server_connector_plugin_name_, kPluginServerConnector, MockServerConnectorFactory);
    ASSERT_EQ(ret, kReturnOk);
    EXPECT_CALL(*server_connector_, Init(testing::_, testing::_)).WillOnce(::testing::Return(kReturnOk));
    EXPECT_CALL(*server_connector_, AsyncReportClient(::testing::_, ::testing::_, ::testing::_))
        .WillRepeatedly(::testing::Return(kReturnOk));
    // 12 times corresponding to 4 internal services, respectively
    EXPECT_CALL(*server_connector_,
                RegisterEventHandler(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
        .Times(::testing::AtMost(12))
        .WillRepeatedly(::testing::DoAll(::testing::Invoke(this, &MockServerConnectorTest::MockIgnoreEventHandler),
                                         ::testing::Return(kReturnOk)));
  }

  virtual void TearDown() {
    for (std::size_t i = 0; i < handler_list_.size(); ++i) {
      delete handler_list_[i];
    }
    handler_list_.clear();
  }

 public:
  void MockIgnoreEventHandler(const ServiceKey& /*service_key*/, ServiceDataType /*data_type*/,
                              uint64_t /*sync_interval*/, const std::string& /*disk_revision*/,
                              ServiceEventHandler* handler) {
    ASSERT_TRUE(handler != NULL);
    const std::lock_guard<std::mutex> mutex_guard(handler_lock_);
    handler_list_.push_back(handler);
  }

  static void* AsyncEventUpdate(void* args) {
    EventHandlerData* event_data = static_cast<EventHandlerData*>(args);
    EXPECT_TRUE(event_data->handler_ != NULL);
    event_data->handler_->OnEventUpdate(event_data->service_key_, event_data->data_type_, event_data->service_data_);
    delete event_data;
    return NULL;
  }

 protected:
  static Plugin* MockServerConnectorFactory() { return server_connector_; }
  static MockServerConnector* server_connector_;
  std::string server_connector_plugin_name_;
  std::mutex handler_lock_;
  std::vector<ServiceEventHandler*> handler_list_;
};

MockServerConnector* MockServerConnectorTest::server_connector_ = NULL;

class TestProviderCallback : public ProviderCallback {
 public:
  TestProviderCallback(ReturnCode ret_code, int line) : ret_code_(ret_code), line_(line) {}

  ~TestProviderCallback() {}

  virtual void Response(ReturnCode code, const std::string&) { ASSERT_EQ(code, ret_code_) << "failed line: " << line_; }

 private:
  ReturnCode ret_code_;
  int line_;
};

class FakeServer {
 public:
  static void SetService(v1::DiscoverResponse& response, const ServiceKey& service_key,
                         const std::string version = "init_version") {
    v1::Service* service = response.mutable_service();
    service->mutable_namespace_()->set_value(service_key.namespace_);
    service->mutable_name()->set_value(service_key.name_);
    service->mutable_revision()->set_value(version);
  }

  static void InstancesResponse(v1::DiscoverResponse& response, const ServiceKey& service_key,
                                const std::string version = "init_version") {
    response.set_type(v1::DiscoverResponse::INSTANCE);
    SetService(response, service_key, version);
  }

  static void RoutingResponse(v1::DiscoverResponse& response, const ServiceKey& service_key,
                              const std::string version = "init_version") {
    response.set_type(v1::DiscoverResponse::ROUTING);
    SetService(response, service_key, version);
  }

  static void LimitResponse(v1::DiscoverResponse& response, const ServiceKey& service_key,
                            const std::string version = "init_version") {
    response.set_type(v1::DiscoverResponse::RATE_LIMIT);
    SetService(response, service_key, version);
  }

  static void CreateServiceInstances(v1::DiscoverResponse& response, const ServiceKey& service_key, int instance_num,
                                     int index_begin = 0) {
    response.Clear();
    response.mutable_code()->set_value(v1::ExecuteSuccess);
    FakeServer::InstancesResponse(response, service_key, "version_one");
    for (int i = 0; i < instance_num; i++) {
      ::v1::Instance* instance = response.add_instances();
      instance->mutable_namespace_()->set_value(service_key.namespace_);
      instance->mutable_service()->set_value(service_key.name_);
      instance->mutable_id()->set_value("instance_" + std::to_string(index_begin + i));
      instance->mutable_host()->set_value("host_" + std::to_string(index_begin + i));
      instance->mutable_port()->set_value(1000 + i);
      instance->mutable_weight()->set_value(100);
      instance->mutable_location()->mutable_region()->set_value("华南");
      instance->mutable_location()->mutable_zone()->set_value("深圳");
      instance->mutable_location()->mutable_campus()->set_value("深圳-大学城");
    }
  }

  static void CreateServiceRoute(v1::DiscoverResponse& response, const ServiceKey& service_key, bool need_router) {
    response.Clear();
    response.mutable_code()->set_value(v1::ExecuteSuccess);
    FakeServer::RoutingResponse(response, service_key, "version_one");
    v1::MatchString exact_string;
    if (need_router) {
      ::v1::Routing* routing = response.mutable_routing();
      routing->mutable_namespace_()->set_value(service_key.namespace_);
      routing->mutable_service()->set_value(service_key.name_);
      ::v1::Route* route = routing->add_inbounds();
      v1::Source* source = route->add_sources();
      source->mutable_namespace_()->set_value(service_key.namespace_);
      source->mutable_service()->set_value(service_key.name_);
      exact_string.mutable_value()->set_value("base");
      (*source->mutable_metadata())["env"] = exact_string;
      for (int i = 0; i < 2; ++i) {
        v1::Destination* destination = route->add_destinations();
        destination->mutable_namespace_()->set_value("*");
        destination->mutable_service()->set_value("*");
        exact_string.mutable_value()->set_value(i == 0 ? "base" : "test");
        (*destination->mutable_metadata())["env"] = exact_string;
        destination->mutable_priority()->set_value(i);
      }
    }
  }

  static ReturnCode InitService(LocalRegistry* local_registry, const ServiceKey& service_key, int instance_num,
                                bool need_router) {
    ReturnCode ret_code;
    ServiceDataNotify* data_notify = NULL;
    ServiceData* service_data = NULL;
    ret_code = local_registry->LoadServiceDataWithNotify(service_key, kServiceDataInstances, service_data, data_notify);
    if (ret_code != kReturnOk) {
      return ret_code;
    }
    ret_code = local_registry->LoadServiceDataWithNotify(service_key, kServiceDataRouteRule, service_data, data_notify);
    if (ret_code != kReturnOk) {
      return ret_code;
    }
    v1::DiscoverResponse response;
    CreateServiceInstances(response, service_key, instance_num);
    service_data = polaris::ServiceData::CreateFromPb(&response, kDataIsSyncing);
    ret_code = local_registry->UpdateServiceData(service_key, kServiceDataInstances, service_data);
    if (ret_code != kReturnOk) {
      return ret_code;
    }
    CreateServiceRoute(response, service_key, need_router);
    service_data = polaris::ServiceData::CreateFromPb(&response, kDataIsSyncing);
    ret_code = local_registry->UpdateServiceData(service_key, kServiceDataRouteRule, service_data);
    if (ret_code != kReturnOk) {
      return ret_code;
    }
    return kReturnOk;
  }

  static void CreateServiceRateLimit(v1::DiscoverResponse& response, const ServiceKey& service_key, int qps) {
    response.Clear();
    response.mutable_code()->set_value(v1::ExecuteSuccess);
    response.set_type(v1::DiscoverResponse::RATE_LIMIT);
    SetService(response, service_key, "version_one");
    v1::RateLimit* rate_limit = response.mutable_ratelimit();
    rate_limit->mutable_revision()->set_value("version_one");
    v1::Rule* rule = rate_limit->add_rules();
    rule->mutable_id()->set_value("4b42d711e0e0414e8bc2567b9140ba09");
    rule->mutable_namespace_()->set_value(service_key.namespace_);
    rule->mutable_service()->set_value(service_key.name_);
    rule->set_type(v1::Rule::LOCAL);
    v1::MatchString match_string;
    match_string.set_type(v1::MatchString::REGEX);
    match_string.mutable_value()->set_value("v*");
    (*rule->mutable_subset())["subset"] = match_string;
    (*rule->mutable_labels())["label"] = match_string;
    v1::Amount* amount = rule->add_amounts();
    amount->mutable_maxamount()->set_value(qps);
    amount->mutable_validduration()->set_seconds(1);
    rule->mutable_revision()->set_value("5483700359f342bcba4421cc58e8a9cd");
  }
};

}  // namespace polaris
