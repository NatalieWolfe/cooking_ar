load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_binary(
  name = "extract",
  srcs = ["extract.cpp"],
  deps = [
    ":pose2d",
    ":pose_extractor",
    "@lw//lw/base",
    "//episode:frames",
    "//episode:project",
  ]
)

cc_library(
  name = "pose2d",
  hdrs = ["pose2d.h"],
  srcs = ["pose2d.cpp"],
  deps = [
    "//third_party:json",
  ],
)

cc_library(
  name = "pose_extractor",
  hdrs = ["pose_extractor.h"],
  srcs = ["pose_extractor.cpp"],
  deps = [
    ":pose2d",
    "@lw//lw/flags",
    "//third_party:opencv",
    "//third_party:openpose",
  ],
)
