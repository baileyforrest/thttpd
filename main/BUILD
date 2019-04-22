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
        "//base",
        "@absl//absl/memory",
        "@absl//absl/strings:str_format",
        "@absl//absl/types:span",
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
