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

#include <utility>

namespace trpc {
//
// Data was read by multi-readers and was writen by single writer.
// It's fine for small piece of data situation, e.g, small diction table, config.
//
// Reference to taf/tars: util/tc_readers_writer_data.h/cpp
//

template <typename T>
class ReadersWriterData {
 public:
  virtual const T& Reader() { return instances_[reader_]; }

  virtual T& Writer() { return instances_[writer_]; }

  // Swap read & write when data was refreshed.
  virtual void Swap() { std::swap(reader_, writer_); }

 public:
  ReadersWriterData() : reader_(0), writer_(1) {}
  virtual ~ReadersWriterData() = default;

 private:
  T instances_[2];
  int reader_{0};
  int writer_{1};
};

}  // namespace trpc
