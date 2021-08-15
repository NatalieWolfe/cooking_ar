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
  branch = "main",
  remote = "https://github.com/google/googletest.git",
)

http_archive(
  name = "opencv",
  strip_prefix = "opencv-4.5.2",
  sha256 = "be976b9ef14f1deaa282fb6e30d75aa8016a2d5c1f08e85795c235148940d753",
  build_file_content =
    'filegroup(name = "all", srcs = glob(["**"]), ' +
    'visibility = ["@//third_party/opencv:__subpackages__"])',
  url = "https://github.com/opencv/opencv/archive/refs/tags/4.5.2.zip",
)

http_archive(
  name = "opencv_contrib",
  strip_prefix = "opencv_contrib-4.5.2",
  sha256 = "8008ac4c623f90f8e67b2d5c58c465616d3317018beca38bd4e39b912fb6e4ae",
  build_file_content =
    'filegroup(name = "modules", srcs = glob(["modules/**"]), ' +
    'visibility = ["@//third_party/opencv:__subpackages__"])',
  url = "https://github.com/opencv/opencv_contrib/archive/refs/tags/4.5.2.zip",
)

http_archive(
    name = "rules_proto",
    strip_prefix = "rules_proto-af6481970a34554c6942d993e194a9aed7987780",
    sha256 = "bc12122a5ae4b517fa423ea03a8d82ea6352d5127ea48cb54bc324e8ab78493c",
    urls = [
        "https://github.com/bazelbuild/rules_proto/archive/af6481970a34554c6942d993e194a9aed7987780.tar.gz",
    ],
)
load("@rules_proto//proto:repositories.bzl", "rules_proto_dependencies", "rules_proto_toolchains")
rules_proto_dependencies()
rules_proto_toolchains()

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
