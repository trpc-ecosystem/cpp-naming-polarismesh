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

#include "trpc/naming/polarismesh/polarismesh_selector_api.h"

#include "trpc/common/trpc_plugin.h"

#include "trpc/naming/polarismesh/polarismesh_selector.h"
#include "trpc/naming/polarismesh/polarismesh_selector_filter.h"

namespace trpc::polarismesh::selector {

bool Init() {
  TrpcPlugin::GetInstance()->RegisterSelector(MakeRefCounted<PolarisMeshSelector>());
  TrpcPlugin::GetInstance()->RegisterClientFilter(std::make_shared<PolarisMeshSelectorFilter>());

  return true;
}

}  // namespace trpc::polarismesh::selector
