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
  name = "fmtlib",
  strip_prefix = "fmt-8.1.1",
  build_file_content =
    'filegroup(name = "all", srcs = glob(["**"]), ' +
    'visibility = ["@//third_party:__subpackages__"])',
  url = "https://github.com/fmtlib/fmt/archive/refs/tags/8.1.1.zip"
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
