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

#include "trpc/naming/polarismesh/polaris_limiter_api.h"

#include "trpc/common/trpc_plugin.h"

#include "trpc/naming/polarismesh/polaris_limiter.h"
#include "trpc/naming/polarismesh/polaris_limiter_client_filter.h"
#include "trpc/naming/polarismesh/polaris_limiter_server_filter.h"

namespace trpc::polarismesh::limiter {

bool Init() {
  TrpcPlugin::GetInstance()->RegisterLimiter(MakeRefCounted<PolarisLimiter>());
  TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<PolarisLimiterClientFilter>());
  TrpcPlugin::GetInstance()->RegisterServerFilter(std::make_shared<PolarisLimiterServerFilter>());

  return true;
}

}  // namespace trpc::polarismesh::limiter
