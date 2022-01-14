load("@rules_cc//cc:defs.bzl", "cc_library")

package(default_visibility = ["//visibility:public"])

cc_library(
  name = "archive",
  hdrs = [
    "include/archive_entry.h",
    "include/archive.h",
  ],
  includes = ["include"],
  linkopts = [
    "-L/usr/local/lib/cmake/depthai/dependencies/lib",
    "-larchive_static",
  ],
)

cc_library(
  name = "bz2",
  hdrs = ["include/bzlib.h"],
  includes = ["include"],
  linkopts = [
    "-L/usr/local/lib/cmake/depthai/dependencies/lib",
    "-lbz2",
  ],
)

cc_library(
  name = "lzma",
  hdrs = [
    "include/lzma.h",
  ] + glob(["include/lzma/**"]),
  includes = ["include"],
  linkopts = [
    "-L/usr/local/lib/cmake/depthai/dependencies/lib",
    "-llzma",
  ],
)

cc_library(
  name = "nlohmann",
  hdrs = glob(["include/nlohmann/**"]),
  includes = ["include"],
)

cc_library(
  name = "nop",
  hdrs = glob(["include/nop/**"]),
  includes = ["include"],
)

cc_library(
  name = "spdlog",
  hdrs = glob(["include/spdlog/**"]),
  includes = ["includes"],
  linkopts = [
    "-L/usr/local/lib/cmake/depthai/dependencies/lib",
    "-lspdlog",
  ],
)

cc_library(
  name = "xlink",
  hdrs = glob(["include/XLink/**"]),
  includes = ["include"],
  linkopts = [
    "-L/usr/local/lib/cmake/depthai/dependencies/lib",
    "-lXLink",
    "-lusb-1.0",
  ],
)

cc_library(
  name = "zlib",
  hdrs = ["include/zlib.h"],
  includes = ["include"],
  linkopts = [
    "-L/usr/local/lib/cmake/depthai/dependencies/lib",
    "-lz",
  ],
)
