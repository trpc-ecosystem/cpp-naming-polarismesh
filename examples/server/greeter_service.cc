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

#include "examples/server/greeter_service.h"

#include <string>

#include "trpc/log/logging.h"

namespace test::helloworld {

::trpc::Status GreeterServiceImpl::SayHello(::trpc::ServerContextPtr context,
                                            const ::trpc::test::helloworld::HelloRequest* request,
                                            ::trpc::test::helloworld::HelloReply* reply) {
  std::string response = "Hello, " + request->msg();
  reply->set_msg(response);

  TRPC_FMT_INFO("receive a request form client");

  return ::trpc::kSuccStatus;
}

}  // namespace test::helloworld
