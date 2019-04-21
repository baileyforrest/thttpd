workspace(name = "calico")

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

local_repository(
    name = "absl",
    path = "third_party/abseil-cpp",
)

local_repository(
    name = "gtest",
    path = "third_party/googletest",
)

# Buildifier setup
http_archive(
    name = "io_bazel_rules_go",
    sha256 = "86ae934bd4c43b99893fc64be9d9fc684b81461581df7ea8fc291c816f5ee8c5",
    urls = ["https://github.com/bazelbuild/rules_go/releases/download/0.18.3/rules_go-0.18.3.tar.gz"],
)

load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains", "go_rules_dependencies")

go_rules_dependencies()

go_register_toolchains()

http_archive(
    name = "com_github_bazelbuild_buildtools",
    sha256 = "96def97196a4d8182de97dccf49afc4097a37d451bc9219b7052e8669421f967",
    strip_prefix = "buildtools-0.22.0",
    url = "https://github.com/bazelbuild/buildtools/archive/0.22.0.zip",
)

load("@com_github_bazelbuild_buildtools//buildifier:deps.bzl", "buildifier_dependencies")

buildifier_dependencies()
