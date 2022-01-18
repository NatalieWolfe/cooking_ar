workspace(name = "cooking_ar")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")
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

git_repository(
  name = "lw",
  branch = "master",
  remote = "https://github.com/LifeWanted/liblw.git",
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
  strip_prefix = "opencv-4.5.3",
  sha256 = "a61e7a4618d353140c857f25843f39b2abe5f451b018aab1604ef0bc34cd23d5",
  build_file_content =
    'filegroup(name = "all", srcs = glob(["**"]), ' +
    'visibility = ["@//third_party:__subpackages__"])',
  url = "https://github.com/opencv/opencv/archive/refs/tags/4.5.3.zip",
)

http_archive(
  name = "opencv_contrib",
  strip_prefix = "opencv_contrib-4.5.3",
  sha256 = "dc3317950cf0d6cab6d24ec8df864d5e7c4efe39627dbd1c7c177dc12a8bcd78",
  build_file_content = 'exports_files(["modules"])',
  url = "https://github.com/opencv/opencv_contrib/archive/refs/tags/4.5.3.zip",
)

new_git_repository(
  name = "openpose",
  tag = "v1.7.0",
  remote = "https://github.com/CMU-Perceptual-Computing-Lab/openpose.git",
  build_file_content =
    'filegroup(name = "all", srcs = glob(["**"]), ' +
    'visibility = ["@//third_party:__subpackages__"])',
  init_submodules = True,
  patches = ["@//third_party:openpose-cdnn-cmake.patch"]
)

http_archive(
  name = "nlohmann_json",
  strip_prefix = "json-3.10.5",
  sha256 = "ea4b0084709fb934f92ca0a68669daa0fe6f2a2c6400bf353454993a834bb0bb",
  build_file_content =
    'filegroup(name = "all", srcs = glob(["**"]), ' +
    'visibility = ["@//third_party:__subpackages__"])',
  url = "https://github.com/nlohmann/json/archive/refs/tags/v3.10.5.zip",
)

new_local_repository(
  name = "depthai_3rdparty",
  path = "/usr/local/include/depthai-shared/3rdparty",
  build_file = "third_party/depthai_3rdparty.BUILD",
)

new_local_repository(
  name = "depthai_dependencies",
  path = "/usr/local/lib/cmake/depthai/dependencies",
  build_file = "third_party/depthai_dependencies.BUILD",
)
