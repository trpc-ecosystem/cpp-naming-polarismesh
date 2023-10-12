# Description: trpc-cpp.

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "common",
    srcs = ["common.cc"],
    hdrs = ["common.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        ":readers_writer_data",
        "//trpc/naming/polarismesh/config:polarismesh_naming_conf",
        "@com_github_polarismesh_polaris//:polarismesh_api_trpc",
        "@trpc_cpp//trpc/codec/trpc",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/common/future",
        "@trpc_cpp//trpc/naming/common:common_defs",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_test(
    name = "common_test",
    srcs = ["common_test.cc"],
    data = [
        "//trpc/naming/polarismesh/testing:polarismesh_test.yaml",
    ],
    linkstatic = True,
    deps = [
        ":common",
        "@com_github_polarismesh_polaris//:polarismesh_api_trpc",
        "@com_google_googletest//:gtest",
        "@trpc_cpp//trpc/naming/common:common_defs",
    ],
)

cc_library(
    name = "mock_polarismesh_api_test",
    srcs = [],
    hdrs = ["mock_polarismesh_api_test.h"],
    deps = [
        "//trpc/naming/polarismesh/config:polarismesh_naming_conf",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_github_polarismesh_polaris//:polarismesh_api_trpc",
    ],
)

cc_library(
    name = "trpc_share_context",
    srcs = ["trpc_share_context.cc"],
    hdrs = ["trpc_share_context.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming/polarismesh:trpc_server_metric",
        "//trpc/naming/polarismesh/config:polarismesh_naming_conf",
        "@com_github_polarismesh_polaris//:polarismesh_api_trpc",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_test(
    name = "trpc_share_context_test",
    srcs = ["trpc_share_context_test.cc"],
    deps = [
        "//trpc/naming/polarismesh:trpc_share_context",
        "//trpc/naming/polarismesh/config:polarismesh_naming_conf",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "polarismesh_selector",
    srcs = ["polarismesh_selector.cc"],
    hdrs = ["polarismesh_selector.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming/polarismesh:common",
        "//trpc/naming/polarismesh:trpc_share_context",
        "//trpc/naming/polarismesh/config:polarismesh_naming_conf",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_github_polarismesh_polaris//:polarismesh_api_trpc",
        "@com_github_tencent_rapidjson//:rapidjson",
        "@trpc_cpp//trpc/codec/trpc",
        "@trpc_cpp//trpc/naming:selector",
        "@trpc_cpp//trpc/naming:selector_factory",
        "@trpc_cpp//trpc/util:string_helper",
        "@trpc_cpp//trpc/util:string_util",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_test(
    name = "polarismesh_selector_test",
    srcs = ["polarismesh_selector_test.cc"],
    data = [
        "//trpc/naming/polarismesh/testing:polarismesh_test.yaml",
    ],
    defines = [] +
              select({
                  "//conditions:default": [],
              }),
    linkstatic = True,
    deps = [
        "//trpc/naming/polarismesh:mock_polarismesh_api_test",
        "//trpc/naming/polarismesh:polarismesh_selector",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_github_polarismesh_polaris//:polarismesh_api_trpc",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@trpc_cpp//trpc/naming:registry_factory",
        "@trpc_cpp//trpc/naming:selector",
        "@trpc_cpp//trpc/naming:selector_factory",
    ],
)

cc_library(
    name = "polarismesh_registry",
    srcs = ["polarismesh_registry.cc"],
    hdrs = ["polarismesh_registry.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming/polarismesh:common",
        "//trpc/naming/polarismesh:trpc_share_context",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_github_polarismesh_polaris//:polarismesh_api_trpc",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/filter:filter_manager",
        "@trpc_cpp//trpc/naming:registry",
        "@trpc_cpp//trpc/naming:registry_factory",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_test(
    name = "polarismesh_registry_test",
    srcs = ["polarismesh_registry_test.cc"],
    linkstatic = True,
    deps = [
        "//trpc/naming/polarismesh:mock_polarismesh_api_test",
        "//trpc/naming/polarismesh:polarismesh_registry",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_google_googletest//:gtest",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/naming:registry",
        "@trpc_cpp//trpc/naming:registry_factory",
    ],
)

cc_library(
    name = "polarismesh_selector_filter",
    srcs = [],
    hdrs = ["polarismesh_selector_filter.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/filter",
        "@trpc_cpp//trpc/naming:selector_workflow",
    ],
)

cc_test(
    name = "polarismesh_selector_filter_test",
    srcs = ["polarismesh_selector_filter_test.cc"],
    deps = [
        ":polarismesh_selector_filter",
        "//trpc/naming/polarismesh:mock_polarismesh_api_test",
        "//trpc/naming/polarismesh:polarismesh_selector",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/filter",
        "@trpc_cpp//trpc/filter:filter_manager",
        "@trpc_cpp//trpc/naming:selector",
        "@trpc_cpp//trpc/naming:selector_factory",
    ],
)

cc_library(
    name = "polarismesh_limiter",
    srcs = ["polarismesh_limiter.cc"],
    hdrs = ["polarismesh_limiter.h"],
    deps = [
        "//trpc/naming/polarismesh:common",
        "//trpc/naming/polarismesh:trpc_share_context",
        "//trpc/naming/polarismesh/config:polarismesh_naming_conf",
        "@com_github_polarismesh_polaris//:polarismesh_api_trpc",
        "@trpc_cpp//trpc/codec/trpc",
        "@trpc_cpp//trpc/naming:limiter",
    ],
)

cc_library(
    name = "polarismesh_limiter_client_filter",
    srcs = ["polarismesh_limiter_client_filter.cc"],
    hdrs = ["polarismesh_limiter_client_filter.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming/polarismesh/config:polarismesh_naming_conf",
        "@trpc_cpp//trpc/client:client_context",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/filter",
        "@trpc_cpp//trpc/naming:limiter",
        "@trpc_cpp//trpc/naming:limiter_factory",
        "@trpc_cpp//trpc/util:time",
    ],
)

cc_library(
    name = "polarismesh_limiter_server_filter",
    srcs = ["polarismesh_limiter_server_filter.cc"],
    hdrs = ["polarismesh_limiter_server_filter.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "//trpc/naming/polarismesh/config:polarismesh_naming_conf",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/filter",
        "@trpc_cpp//trpc/naming:limiter",
        "@trpc_cpp//trpc/naming:limiter_factory",
        "@trpc_cpp//trpc/server:server_context",
        "@trpc_cpp//trpc/util:time",
    ],
)

cc_test(
    name = "polarismesh_limiter_test",
    srcs = ["polarismesh_limiter_test.cc"],
    data = [
    ],
    linkstatic = True,
    deps = [
        "//trpc/naming/polarismesh:mock_polarismesh_api_test",
        "//trpc/naming/polarismesh:polarismesh_limiter",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/filter:filter_manager",
        "@trpc_cpp//trpc/naming:limiter_factory",
    ],
)

cc_test(
    name = "polarismesh_limiter_client_filter_test",
    srcs = ["polarismesh_limiter_client_filter_test.cc"],
    data = ["//trpc/naming/polarismesh/testing:polarismesh_test.yaml"],
    deps = [
        ":polarismesh_limiter",
        "//trpc/naming/polarismesh:mock_polarismesh_api_test",
        "//trpc/naming/polarismesh:polarismesh_limiter_client_filter",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/util:time",
    ],
)

cc_test(
    name = "polarismesh_limiter_server_filter_test",
    srcs = ["polarismesh_limiter_server_filter_test.cc"],
    data = ["//trpc/naming/polarismesh/testing:polarismesh_test.yaml"],
    deps = [
        ":polarismesh_limiter",
        "//trpc/naming/polarismesh:mock_polarismesh_api_test",
        "//trpc/naming/polarismesh:polarismesh_limiter_server_filter",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@trpc_cpp//trpc/codec/trpc:trpc_protocol",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/server:rpc_service_impl",
        "@trpc_cpp//trpc/server/testing:service_adapter_testing",
    ],
)

cc_library(
    name = "trpc_server_metric",
    srcs = ["trpc_server_metric.cc"],
    hdrs = ["trpc_server_metric.h"],
    visibility = [
        "//visibility:public",
    ],
    deps = [
        "@trpc_cpp//trpc/codec/trpc",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@com_github_polarismesh_polaris//:polarismesh_api_trpc",
        "@trpc_cpp//trpc/metrics:metrics_factory",
        "@trpc_cpp//trpc/metrics:trpc_metrics",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_test(
    name = "trpc_server_metric_test",
    srcs = ["trpc_server_metric_test.cc"],
    data = ["//trpc/naming/polarismesh/testing:polarismesh_test.yaml"],
    deps = [
        "//trpc/naming/polarismesh:trpc_server_metric",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/metrics:metrics_factory",
        "@trpc_cpp//trpc/metrics:trpc_metrics",
    ],
)

cc_library(
    name = "readers_writer_data",
    hdrs = ["readers_writer_data.h"],
)

cc_library(
    name = "polarismesh_selector_api",
    srcs = ["polarismesh_selector_api.cc"],
    hdrs = ["polarismesh_selector_api.h"],
    deps = [
        "//trpc/naming/polarismesh:polarismesh_selector",
        "//trpc/naming/polarismesh:polarismesh_selector_filter",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/naming:selector_factory",
    ],
)

cc_library(
    name = "polarismesh_limiter_api",
    srcs = ["polarismesh_limiter_api.cc"],
    hdrs = ["polarismesh_limiter_api.h"],
    deps = [
        "//trpc/naming/polarismesh:polarismesh_limiter",
        "//trpc/naming/polarismesh:polarismesh_limiter_client_filter",
        "//trpc/naming/polarismesh:polarismesh_limiter_server_filter",
        "@trpc_cpp//trpc/common:trpc_plugin",
    ],
)

cc_library(
    name = "polarismesh_registry_api",
    srcs = ["polarismesh_registry_api.cc"],
    hdrs = ["polarismesh_registry_api.h"],
    deps = [
        "//trpc/naming/polarismesh:polarismesh_registry",
        "@trpc_cpp//trpc/common:trpc_plugin",
    ],
)
