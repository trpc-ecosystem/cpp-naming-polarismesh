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

#include "trpc/naming/polarismesh/trpc_share_context.h"

#include "gtest/gtest.h"

#include "trpc/naming/polarismesh/config/polaris_naming_conf.h"

namespace trpc {

TEST(TrpcShareContext, InitAndDestory) {
  trpc::TrpcShareContext& trpc_share_context = *trpc::TrpcShareContext::GetInstance();

  // Illegal Config enters the paragraph, and the creation failed
  trpc::naming::PolarisNamingConfig bad_config;
  bad_config.orig_selector_config = "[,,,";
  ASSERT_EQ(-1, trpc_share_context.Init(bad_config));

  // The number of parameters in Config can not create Context, and the creation failed
  bad_config.orig_selector_config =
      "global:\n"
      "  serverConnector:\n"
      "    protocol: not_exist\n"
      "    addresses:\n"
      "    - 127.0.0.1:8091\n";
  ASSERT_EQ(-1, trpc_share_context.Init(bad_config));
  // The creation failed, the server_connector obtained
  ASSERT_TRUE(trpc_share_context.GetServerConnector() == nullptr);

  // The correct config enters the parameters, and the creation is successful
  trpc::naming::PolarisNamingConfig right_config;
  right_config.orig_selector_config =
      "global:\n"
      "  serverConnector:\n"
      "    protocol: grpc\n"
      "    addresses:\n"
      "    - 127.0.0.1:8091\n";
  ASSERT_EQ(0, trpc_share_context.Init(right_config));
  // Repeat creation, return directly to success
  ASSERT_EQ(0, trpc_share_context.Init(right_config));
  // After the creation is successful, neither of the Context and Server_Connector
  ASSERT_TRUE(trpc_share_context.GetPolarisContext() != nullptr);
  ASSERT_TRUE(trpc_share_context.GetServerConnector() != nullptr);

  // After destruction, the context you get is empty
  trpc_share_context.Destroy();
  ASSERT_TRUE(trpc_share_context.GetPolarisContext() == nullptr);
}

}  // namespace trpc
