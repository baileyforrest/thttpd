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
    name = "mpsc-queue",
    hdrs = [
        "mpsc-queue.h",
    ],
    deps = [
        "@absl//absl/base",
        "@absl//absl/synchronization",
    ],
)

cc_test(
    name = "mpsc-queue_test",
    srcs = [
        "mpsc-queue_test.cc",
    ],
    deps = [
        ":mpsc-queue",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "scoped-fd",
    hdrs = [
        "scoped-fd.h",
    ],
)

cc_test(
    name = "scoped-fd_test",
    srcs = [
        "scoped-fd_test.cc",
    ],
    deps = [
        ":scoped-fd",
        "@absl//absl/memory",
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
    deps = [
        ":mpsc-queue",
    ],
)

cc_test(
    name = "task-runner_test",
    srcs = [
        "task-runner_test.cc",
    ],
    deps = [
        ":task-runner",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "util",
    srcs = [
        "util.cc",
    ],
    hdrs = [
        "util.h",
    ],
    deps = [
        ":base",
        "@absl//absl/base",
        "@absl//absl/strings",
    ],
)
