load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_binary(
  name = "extract",
  srcs = ["extract.cpp"],
  deps = [
    ":pose2d",
    ":pose3d",
    ":pose_extractor",
    ":triangulation",
    "@lw//lw/base",
    "//cli:progress",
    "//episode:cameras",
    "//episode:frames",
    "//episode:project",
  ]
)

cc_library(
  name = "pose2d",
  hdrs = ["pose2d.h"],
  srcs = ["pose2d.cpp"],
  deps = [
    "@lw//lw/err",
    "//third_party:json",
  ],
)

cc_library(
  name = "pose3d",
  hdrs = ["pose3d.h"],
  srcs = ["pose3d.cpp"],
  deps = [
    "@lw//lw/err",
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

cc_library(
  name = "triangulation",
  hdrs = ["triangulation.h"],
  srcs = ["triangulation.cpp"],
  deps = [
    ":pose2d",
    ":pose3d",
    "//episode:cameras",
    "//third_party:opencv",
  ],
)

cc_binary(
  name = "visualize",
  srcs = ["visualize.cpp"],
  deps = [
    ":pose2d",
    "//cli:keys",
    "//episode:frames",
    "//episode:project",
    "//third_party:opencv",
  ],
)
