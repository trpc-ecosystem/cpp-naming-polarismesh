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

#include <memory>
#include <mutex>

#include "polaris/context.h"
#include "polaris/plugin/server_connector/server_connector.h"
#include "polaris/polaris.h"

#include "trpc/naming/polarismesh/config/polaris_naming_conf.h"

namespace trpc {

/// @brief Maintain the shared type Polaris Context, which Context is used to create other SDK API interface objects
class TrpcShareContext {
 public:
  static TrpcShareContext* GetInstance() {
    static TrpcShareContext instance;
    return &instance;
  }

  TrpcShareContext(const TrpcShareContext&) = delete;
  TrpcShareContext& operator=(const TrpcShareContext&) = delete;

  /// @brief Initialize the polarismesh according to the framework configuration file
  /// @param polaris_naming_config polarismesh plugin configuration
  /// @return int Success is 0, failure is -1
  int Init(const trpc::naming::PolarisNamingConfig& polaris_naming_config);

  /// @brief Destruction of polarismesh Context
  void Destroy();

  /// @brief Get the polarismesh context
  /// @note Applicable scenario: The SDK API method that needs to be called by the business is unpacking the packaging
  ///       How to use: Consumer_api, Provider_API or Limit_api
  ///       Note: Don't try the context obtained by the destruction, which will make the polarismesh plug -in function
  ///       fail
  /// @return std::shared_ptr<polarismesh::Context> Quote from the polarismesh Context
  std::shared_ptr<polaris::Context> GetPolarisContext() { return polaris_context_; }

  /// @brief Get the Server Connector object inside SDK, and the integrated test will call
  /// @return ServerConnector* References of the polarismesh Internal Server Connector object
  polaris::ServerConnector* GetServerConnector();

 private:
  TrpcShareContext() = default;

 private:
  bool init_{false};
  std::mutex mutex_;
  std::shared_ptr<polaris::Context> polaris_context_{nullptr};
};

}  // namespace trpc
