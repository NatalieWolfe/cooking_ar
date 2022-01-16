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
    "-lusb-1.0",
  ],
  deps = [
    ":opencv",
    "@depthai_3rdparty//:tl",
    "@depthai_dependencies//:archive",
    "@depthai_dependencies//:bz2",
    "@depthai_dependencies//:lzma",
    "@depthai_dependencies//:nlohmann",
    "@depthai_dependencies//:nop",
    "@depthai_dependencies//:spdlog",
    "@depthai_dependencies//:xlink",
    "@depthai_dependencies//:zlib",
  ],
)

cmake(
  name = "json",
  visibility = ["//visibility:public"],
  lib_source = "@nlohmann_json//:all",
  cache_entries = {
    "JSON_BuildTests": "Off",
    "JSON_Install": "On",
  },
  out_headers_only = True,
)
