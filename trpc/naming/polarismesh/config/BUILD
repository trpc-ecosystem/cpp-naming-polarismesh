# Description: trpc-cpp.

load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "default_naming_conf",
    srcs = ["default_naming_conf.cc"],
    hdrs = ["default_naming_conf.h"],
    deps = [
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_test(
    name = "default_naming_conf_test",
    srcs = ["default_naming_conf_test.cc"],
    deps = [
        ":default_naming_conf",
        "@com_google_googletest//:gtest_main",
    ],
)

cc_library(
    name = "polaris_naming_conf",
    srcs = ["polaris_naming_conf.cc"],
    hdrs = ["polaris_naming_conf.h"],
    deps = [
        ":default_naming_conf",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@trpc_cpp//trpc/client:client_context",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_test(
    name = "polaris_naming_conf_test",
    srcs = ["polaris_naming_conf_test.cc"],
    copts = [
        "-g",
    ],
    linkstatic = True,
    deps = [
        ":polaris_naming_conf",
        "//trpc/naming/polarismesh:mock_polaris_api_test",
        "@com_github_jbeder_yaml_cpp//:yaml-cpp",
        "@com_github_polarismesh_polaris//:polaris_api_trpc",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
