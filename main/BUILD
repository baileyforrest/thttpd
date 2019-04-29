cc_binary(
    name = "thttpd",
    srcs = [
        "main.cc",
    ],
    deps = [
        ":libthttpd",
        "//base",
        "//base:util",
    ],
)

cc_library(
    name = "config",
    hdrs = [
        "config.h",
    ],
)

cc_library(
    name = "content-type",
    srcs = [
        "content-type.cc",
    ],
    hdrs = [
        "content-type.h",
    ],
    deps = [
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/strings",
    ],
)

cc_library(
    name = "file-reader",
    srcs = [
        "file-reader.cc",
    ],
    hdrs = [
        "file-reader.h",
    ],
    deps = [
        "//base",
        "@absl//absl/strings",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "http-request",
    srcs = [
        "http-request.cc",
    ],
    hdrs = [
        "http-request.h",
    ],
    deps = [
        "//base",
        "@absl//absl/base",
        "@absl//absl/strings",
    ],
)

cc_library(
    name = "http-response",
    srcs = [
        "http-response.cc",
    ],
    hdrs = [
        "http-response.h",
    ],
    deps = [
        "//base",
        "@absl//absl/base",
        "@absl//absl/strings",
    ],
)

cc_library(
    name = "libthttpd",
    srcs = [
        "request-handler.cc",
        "request-handler.h",
        "thttpd.cc",
    ],
    hdrs = [
        "thttpd.h",
    ],
    deps = [
        ":config",
        ":content-type",
        ":file-reader",
        ":http-response",
        ":request-parser",
        ":thread-pool",
        "//base",
        "//base:task-runner",
        "//base:util",
        "@absl//absl/container:flat_hash_map",
        "@absl//absl/memory",
        "@absl//absl/types:optional",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "request-parser",
    srcs = [
        "request-parser.cc",
    ],
    hdrs = [
        "request-parser.h",
    ],
    deps = [
        ":http-request",
        "//base",
        "@absl//absl/strings",
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
