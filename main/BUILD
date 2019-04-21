package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "thttpd",
    srcs = [
        "main.cc",
    ],
    deps = [
        "@absl//absl/memory",
        "@absl//absl/strings",
        "@absl//absl/types:span",
    ],
)

cc_library(
    name = "foo",
    srcs = [
        "foo.cc",
    ],
    hdrs = [
        "foo.h",
    ],
)

cc_test(
    name = "foo_test",
    srcs = [
        "foo_test.cc",
    ],
    deps = [
        ":foo",
        "@gtest//:gtest_main",
    ],
)
