cc_binary(
    name = "thttpd",
    srcs = [
        "main.cc",
    ],
    deps = [
        ":libthttpd",
        "//base",
    ],
)

cc_library(
    name = "libthttpd",
    srcs = [
        "thttpd.cc",
    ],
    hdrs = [
        "thttpd.h",
    ],
    deps = [
        ":request-handler",
        ":thread-pool",
        "//base",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/memory",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "request-handler",
    srcs = [
        "request-handler.cc",
    ],
    hdrs = [
        "request-handler.h",
    ],
    deps = [
        "//base",
        "//base:task-runner",
        "@absl//absl/memory",
    ],
)

cc_library(
    name = "thread-pool",
    srcs = [
        "thread-pool.cc",
    ],
    hdrs = [
        "thread-pool.h",
    ],
    deps = [
        "//base:task-runner",
        "@absl//absl/memory",
    ],
)

cc_test(
    name = "thread-pool_test",
    srcs = [
        "thread-pool_test.cc",
    ],
    deps = [
        ":thread-pool",
        "@gtest//:gtest_main",
    ],
)
