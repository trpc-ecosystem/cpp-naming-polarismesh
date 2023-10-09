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
#include "trpc/util/log/logging.h"

namespace trpc::naming {

void ServiceConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("name:" << name);
  TRPC_LOG_DEBUG("namespace:" << namespace_);
  TRPC_LOG_DEBUG("token:" << token);
  TRPC_LOG_DEBUG("instance_id:" << instance_id);
  TRPC_LOG_DEBUG("metadata_size:" << metadata.size());

  TRPC_LOG_DEBUG("--------------------------------");
}

void RegistryConfig::Display() const {
  TRPC_LOG_DEBUG("--------------------------------");

  TRPC_LOG_DEBUG("heartbeat_interval:" << heartbeat_interval);
  TRPC_LOG_DEBUG("heartbeat_timeout:" << heartbeat_timeout);
  for (auto& item : services_config) {
    item.Display();
  }

  TRPC_LOG_DEBUG("--------------------------------");
}

}  // namespace trpc::naming