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

#include <string>
#include <vector>

#include "yaml-cpp/yaml.h"

namespace trpc::naming {

struct ServiceConfig {
  std::string name;
  std::string namespace_;
  std::string token;
  std::string instance_id;
  std::map<std::string, std::string> metadata;

  void Display() const;
};

struct RegistryConfig {
  uint64_t heartbeat_interval{3000};
  uint64_t heartbeat_timeout{2000};
  std::vector<ServiceConfig> services_config;

  void Display() const;
};

}  // namespace trpc::naming

namespace YAML {

template <>
struct convert<trpc::naming::ServiceConfig> {
  static YAML::Node encode(const trpc::naming::ServiceConfig& config) {
    YAML::Node node;

    node["name"] = config.name;

    node["namespace"] = config.namespace_;

    node["token"] = config.token;

    node["instance_id"] = config.instance_id;

    node["metadata"] = config.metadata;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::ServiceConfig& config) {
    if (node["name"]) {
      config.name = node["name"].as<std::string>();
    }

    if (node["namespace"]) {
      config.namespace_ = node["namespace"].as<std::string>();
    }

    if (node["token"]) {
      config.token = node["token"].as<std::string>();
    }

    if (node["instance_id"]) {
      config.instance_id = node["instance_id"].as<std::string>();
    }

    if (node["metadata"]) {
      config.metadata = node["metadata"].as<std::map<std::string, std::string>>();
    }

    return true;
  }
};

template <>
struct convert<trpc::naming::RegistryConfig> {
  static YAML::Node encode(const trpc::naming::RegistryConfig& config) {
    YAML::Node node;

    node["heartbeat_interval"] = config.heartbeat_interval;

    node["heartbeat_timeout"] = config.heartbeat_timeout;

    node["service"] = config.services_config;

    return node;
  }

  static bool decode(const YAML::Node& node, trpc::naming::RegistryConfig& config) {
    if (node["heartbeat_interval"]) {
      config.heartbeat_interval = node["heartbeat_interval"].as<uint64_t>();
    }

    if (node["heartbeat_timeout"]) {
      config.heartbeat_timeout = node["heartbeat_timeout"].as<uint64_t>();
    }

    if (node["service"]) {
      config.services_config = node["service"].as<std::vector<trpc::naming::ServiceConfig>>();
    }

    return true;
  }
};

}  // namespace YAML