package(default_visibility = ["//visibility:public"])

cc_library(
    name = "base",
    srcs = [
        "logging.cc",
    ],
    hdrs = [
        "err.h",
        "logging.h",
        "scoped-destructor.h",
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
    name = "file-reader",
    srcs = [
        "file-reader.cc",
    ],
    hdrs = [
        "file-reader.h",
    ],
    deps = [
        ":base",
        ":reader",
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
    name = "once-callback",
    srcs = [
        "once-callback-internal.h",
    ],
    hdrs = [
        "once-callback.h",
    ],
    deps = [
        "@absl//absl/base",
    ],
)

cc_test(
    name = "once-callback_test",
    srcs = [
        "once-callback_test.cc",
    ],
    deps = [
        ":once-callback",
        "@absl//absl/memory",
        "@gtest//:gtest_main",
    ],
)

cc_library(
    name = "reader",
    hdrs = [
        "reader.h",
    ],
    deps = [
        ":base",
        "@absl//absl/types:span",
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
    name = "stdin-reader",
    hdrs = [
        "stdin-reader.h",
    ],
    deps = [
        ":base",
        ":reader",
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
        ":once-callback",
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

cc_library(
    name = "zlib-deflate-reader",
    srcs = [
        "zlib-deflate-reader.cc",
    ],
    hdrs = [
        "zlib-deflate-reader.h",
    ],
    linkopts = ["-lz"],
    deps = [
        ":base",
        ":reader",
    ],
)

cc_binary(
    name = "zlib-deflate-reader_test",
    srcs = [
        "zlib-deflate-reader_test.cc",
    ],
    linkopts = ["-lz"],
    deps = [
        ":stdin-reader",
        ":zlib-deflate-reader",
    ],
)
