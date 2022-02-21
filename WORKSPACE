workspace(name = "cooking_ar")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")
load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

# Load the rules_foreign_cc tooling.
git_repository(
  name = "rules_foreign_cc",
  tag = "0.7.1",
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
  commit = "56e7eb3c5ac7a6efbf392554cbd21ef42ab557f3",
  remote = "https://github.com/LifeWanted/liblw.git",
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
  build_file_content =
    'filegroup(name = "modules", srcs = glob(["modules/**"]), ' +
    'visibility=["@//third_party:__subpackages__"])',
  url = "https://github.com/opencv/opencv_contrib/archive/refs/tags/4.5.3.zip",
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

git_repository(
  name = "com_google_protobuf",
  tag = "v3.19.4",
  remote = "https://github.com/protocolbuffers/protobuf.git",
)
load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")
protobuf_deps()

# ---------------------------------------------------------------------------- #
# Mediapipe WORKSPACE.

git_repository(
  name = "mediapipe",
  branch = "patch",
  remote = "https://github.com/NatalieWolfe/mediapipe.git",
)

http_archive(
  name = "bazel_skylib",
  type = "tar.gz",
  urls = [
    "https://github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
    "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.0.3/bazel-skylib-1.0.3.tar.gz",
  ],
  sha256 = "1c531376ac7e5a180e0237938a2536de0c54d93f5c278634818e0efc952dd56c",
)
load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")
bazel_skylib_workspace()
load("@bazel_skylib//lib:versions.bzl", "versions")
versions.check(minimum_bazel_version = "3.7.2")

# gflags needed by glog
http_archive(
  name = "com_github_gflags_gflags",
  strip_prefix = "gflags-2.2.2",
  sha256 = "19713a36c9f32b33df59d1c79b4958434cb005b5b47dc5400a7a4b078111d9b5",
  url = "https://github.com/gflags/gflags/archive/v2.2.2.zip",
)

# 2020-08-21
http_archive(
  name = "com_github_glog_glog",
  strip_prefix = "glog-0a2e5931bd5ff22fd3bf8999eb8ce776f159cda6",
  sha256 = "58c9b3b6aaa4dd8b836c0fd8f65d0f941441fb95e27212c5eeb9979cfd3592ab",
  urls = [
    "https://github.com/google/glog/archive/0a2e5931bd5ff22fd3bf8999eb8ce776f159cda6.zip",
  ],
)
http_archive(
  name = "com_github_glog_glog_no_gflags",
  strip_prefix = "glog-0a2e5931bd5ff22fd3bf8999eb8ce776f159cda6",
  sha256 = "58c9b3b6aaa4dd8b836c0fd8f65d0f941441fb95e27212c5eeb9979cfd3592ab",
  build_file = "@mediapipe//third_party:glog_no_gflags.BUILD",
  urls = [
    "https://github.com/google/glog/archive/0a2e5931bd5ff22fd3bf8999eb8ce776f159cda6.zip",
  ],
  patches = [
    "@mediapipe//third_party:com_github_glog_glog_9779e5ea6ef59562b030248947f787d1256132ae.diff",
  ],
  patch_args = [
    "-p1",
  ],
)

# easyexif
http_archive(
  name = "easyexif",
  url = "https://github.com/mayanklahiri/easyexif/archive/master.zip",
  strip_prefix = "easyexif-master",
  build_file = "@mediapipe//third_party:easyexif.BUILD",
)

# libyuv
http_archive(
  name = "libyuv",
  # Error: operand type mismatch for `vbroadcastss' caused by commit 8a13626e42f7fdcf3a6acbb0316760ee54cda7d8.
  urls = ["https://chromium.googlesource.com/libyuv/libyuv/+archive/2525698acba9bf9b701ba6b4d9584291a1f62257.tar.gz"],
  build_file = "@mediapipe//third_party:libyuv.BUILD",
)

http_archive(
  name = "com_google_audio_tools",
  strip_prefix = "multichannel-audio-tools-master",
  urls = ["https://github.com/google/multichannel-audio-tools/archive/master.zip"],
)

# Point to the commit that deprecates the usage of Eigen::MappedSparseMatrix.
http_archive(
  name = "ceres_solver",
  url = "https://github.com/ceres-solver/ceres-solver/archive/123fba61cf2611a3c8bddc9d91416db26b10b558.zip",
  patches = [
    "@mediapipe//third_party:ceres_solver_compatibility_fixes.diff"
  ],
  patch_args = [
    "-p1",
  ],
  strip_prefix = "ceres-solver-123fba61cf2611a3c8bddc9d91416db26b10b558",
  sha256 = "8b7b16ceb363420e0fd499576daf73fa338adb0b1449f58bea7862766baa1ac7"
)

new_local_repository(
  name = "linux_opencv",
  build_file = "@mediapipe//third_party:opencv_linux.BUILD",
  path = "/usr",
)

new_local_repository(
  name = "linux_ffmpeg",
  build_file = "@mediapipe//third_party:ffmpeg_linux.BUILD",
  path = "/usr"
)

new_local_repository(
  name = "macos_opencv",
  build_file = "@mediapipe//third_party:opencv_macos.BUILD",
  path = "/usr/local/opt/opencv@3",
)

new_local_repository(
  name = "macos_ffmpeg",
  build_file = "@mediapipe//third_party:ffmpeg_macos.BUILD",
  path = "/usr/local/opt/ffmpeg",
)

new_local_repository(
  name = "windows_opencv",
  build_file = "@mediapipe//third_party:opencv_windows.BUILD",
  path = "C:\\opencv\\build",
)

http_archive(
  name = "android_opencv",
  build_file = "@mediapipe//third_party:opencv_android.BUILD",
  strip_prefix = "OpenCV-android-sdk",
  type = "zip",
  url = "https://github.com/opencv/opencv/releases/download/3.4.3/opencv-3.4.3-android-sdk.zip",
)

http_archive(
  name = "stblib",
  strip_prefix = "stb-b42009b3b9d4ca35bc703f5310eedc74f584be58",
  sha256 = "13a99ad430e930907f5611325ec384168a958bf7610e63e60e2fd8e7b7379610",
  urls = ["https://github.com/nothings/stb/archive/b42009b3b9d4ca35bc703f5310eedc74f584be58.tar.gz"],
  build_file = "@mediapipe//third_party:stblib.BUILD",
  patches = [
    "@mediapipe//third_party:stb_image_impl.diff"
  ],
  patch_args = [
    "-p1",
  ],
)

# Needed by TensorFlow
http_archive(
  name = "io_bazel_rules_closure",
  sha256 = "e0a111000aeed2051f29fcc7a3f83be3ad8c6c93c186e64beb1ad313f0c7f9f9",
  strip_prefix = "rules_closure-cf1e44edb908e9616030cc83d085989b8e6cd6df",
  urls = [
    "http://mirror.tensorflow.org/github.com/bazelbuild/rules_closure/archive/cf1e44edb908e9616030cc83d085989b8e6cd6df.tar.gz",
    "https://github.com/bazelbuild/rules_closure/archive/cf1e44edb908e9616030cc83d085989b8e6cd6df.tar.gz",  # 2019-04-04
  ],
)

# Tensorflow repo should always go after the other external dependencies.
# 2021-12-02
_TENSORFLOW_GIT_COMMIT = "3f878cff5b698b82eea85db2b60d65a2e320850e"
_TENSORFLOW_SHA256 = "21d919ad6d96fcc0477c8d4f7b1f7e4295aaec2986e035551ed263c2b1cd52ee"
http_archive(
  name = "org_tensorflow",
  urls = [
    "https://github.com/tensorflow/tensorflow/archive/%s.tar.gz" % _TENSORFLOW_GIT_COMMIT,
  ],
  patches = [
    "@mediapipe//third_party:org_tensorflow_compatibility_fixes.diff",
    "@mediapipe//third_party:org_tensorflow_objc_cxx17.diff",
    # Diff is generated with a script, don't update it manually.
    "@mediapipe//third_party:org_tensorflow_custom_ops.diff",
  ],
  patch_args = [
    "-p1",
  ],
  strip_prefix = "tensorflow-%s" % _TENSORFLOW_GIT_COMMIT,
  sha256 = _TENSORFLOW_SHA256,
)

load("@org_tensorflow//tensorflow:workspace3.bzl", "tf_workspace3")
tf_workspace3()
load("@org_tensorflow//tensorflow:workspace2.bzl", "tf_workspace2")
tf_workspace2()

# Edge TPU
http_archive(
  name = "libedgetpu",
  sha256 = "14d5527a943a25bc648c28a9961f954f70ba4d79c0a9ca5ae226e1831d72fe80",
  strip_prefix = "libedgetpu-3164995622300286ef2bb14d7fdc2792dae045b7",
  urls = [
    "https://github.com/google-coral/libedgetpu/archive/3164995622300286ef2bb14d7fdc2792dae045b7.tar.gz"
  ],
)
load("@libedgetpu//:workspace.bzl", "libedgetpu_dependencies")
libedgetpu_dependencies()

load("@coral_crosstool//:configure.bzl", "cc_crosstool")
cc_crosstool(name = "crosstool")
