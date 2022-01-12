load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

cc_library(
  name = "opencv",
  visibility = ["//visibility:public"],
  linkopts = [
    "-L/usr/local/lib",
    "-pthread",
    "-lopencv_world",
  ],
)

cc_library(
  name = "openpose",
  visibility = ["//visibility:public"],
  linkopts = ["-lopenpose"],
)

# Must build and install https://github.com/luxonis/depthai-core separately.
cc_library(
  name = "depthai",
  visibility = ["//visibility:public"],
  linkopts = [
    "-L/usr/local/lib",
    "-ldepthai-core",
    "-ldepthai-opencv",
    "-ldepthai-resources",
  ],
)

cmake(
  name = "fmtlib",
  # cache_entries = {
  #   "CMAKE_C_FLAGS": "-fPIC",
  # },
  lib_source = "@fmtlib//:all",
  out_static_libs = ["libfmt.a"],
)
