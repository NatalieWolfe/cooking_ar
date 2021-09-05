load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
  name = "camera_stream",
  srcs = ["camera_stream.cpp"],
  hdrs = ["camera_stream.h"],
)

cc_library(
  name = "frame_saver",
  srcs = ["frame_saver.cpp"],
  hdrs = ["frame_saver.h"],
  deps = [":camera_stream"],
)
