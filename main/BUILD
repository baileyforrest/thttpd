package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "foo-maker",
    srcs = [
        "main.cc",
    ],
    deps = [
        ":foo",
        "@absl//absl/memory",
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
