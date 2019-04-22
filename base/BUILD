package(default_visibility = ["//visibility:public"])

cc_library(
    name = "base",
    srcs = [
        "logging.cc",
    ],
    hdrs = [
        "err.h",
        "logging.h",
    ],
    deps = [
        "@absl//absl/base",
        "@absl//absl/strings",
    ],
)

cc_test(
    name = "err_test",
    srcs = [
        "err_test.cc",
    ],
    deps = [
        ":base",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "task-runner",
    srcs = [
        "task-runner.cc",
    ],
    hdrs = [
        "task-runner.h",
    ],
)
