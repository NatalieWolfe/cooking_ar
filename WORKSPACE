workspace(name = "cooking_ar")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Load the rules_foreign_cc tooling.
git_repository(
  name = "rules_foreign_cc",
  branch = "main",
  remote = "https://github.com/bazelbuild/rules_foreign_cc.git",
)
load(
  "@rules_foreign_cc//foreign_cc:repositories.bzl",
  "rules_foreign_cc_dependencies",
)
rules_foreign_cc_dependencies()

git_repository(
  name = "gtest",
  branch = "master",
  remote = "https://github.com/google/googletest.git",
)

http_archive(
    name = "com_google_protobuf",
    strip_prefix = "protobuf-master",
    urls = ["https://github.com/protocolbuffers/protobuf/archive/master.zip"],
)
load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()

http_archive(
    name = "rules_cc",
    strip_prefix = "rules_cc-daf6ace7cfeacd6a83e9ff2ed659f416537b6c74",
    sha256 = "34b2ebd4f4289ebbc27c7a0d854dcd510160109bb0194c0ba331c9656ffcb556",
    urls = [
        "https://github.com/bazelbuild/rules_cc/archive/daf6ace7cfeacd6a83e9ff2ed659f416537b6c74.tar.gz",
    ],
)
load("@rules_cc//cc:repositories.bzl", "rules_cc_dependencies")
rules_cc_dependencies()
