load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_library(
  name = "camera_controller",
  srcs = ["camera_controller.cpp"],
  hdrs = ["camera_controller.h"],
  deps = [
    ":camera_stream",
    ":frame_saver",
    "//third_party:opencv",
  ],
)

cc_library(
  name = "camera_stream",
  srcs = ["camera_stream.cpp"],
  hdrs = ["camera_stream.h"],
)

cc_library(
  name = "charuco",
  srcs = ["charuco.cpp"],
  hdrs = ["charuco.h"],
  deps = ["//third_party:opencv"],
)

cc_library(
  name = "frame_saver",
  srcs = ["frame_saver.cpp"],
  hdrs = ["frame_saver.h"],
  deps = [":camera_stream"],
)

cc_library(
  name = "input",
  srcs = ["input.cpp"],
  hdrs = ["input.h"],
  deps = ["//third_party:opencv"],
)

cc_binary(
  name = "monitor",
  srcs = ["monitor.cpp"],
  deps = [
    ":camera_controller",
    ":charuco",
    ":input",
  ],
)
