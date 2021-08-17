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
