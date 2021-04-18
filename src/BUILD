load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_binary(
  name = "calibrator",
  srcs = ["calibrator.cpp"],
  linkopts = [
    "-L/usr/local/lib",
    "-pthread",
    "-lopencv_world",
  ],
)

cc_binary(
  name = "extractor",
  srcs = ["extractor.cpp"],
  linkopts = [
    "-L/usr/local/lib",
    "-pthread",
    "-lopencv_world",
    "-lopenpose",
  ],
  deps = [":timing"],
)

cc_binary(
  name = "recorder",
  srcs = ["recorder.cpp"],
  linkopts = [
    "-L/usr/local/lib",
    "-pthread",
    "-lopencv_world",
  ],
  deps = [":timing"],
)

cc_library(
  name = "timing",
  hdrs = ["timing.h"],
  srcs = ["timing.cpp"],
)

cc_binary(
  name = "track",
  srcs = ["track.cpp"],
  linkopts = [
    "-L/usr/local/lib",
    "-pthread",
    "-lopencv_aruco",
    "-lopencv_calib3d",
    "-lopencv_core",
    "-lopencv_highgui",
    "-lopencv_imgcodecs",
    "-lopencv_imgproc",
    "-lopencv_videoio",
    "-lopencv_world",
  ],
)
