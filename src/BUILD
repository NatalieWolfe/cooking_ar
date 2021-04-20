load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library")

cc_binary(
  name = "calibrator",
  srcs = ["calibrator.cpp"],
  deps = [
    ":cameras",
    ":files",
    "//third_party:opencv",
  ],
)

cc_library(
  name = "cameras",
  hdrs = ["cameras.h"],
  srcs = ["cameras.cpp"],
  deps = ["//third_party:opencv"],
)

cc_binary(
  name = "extractor",
  srcs = ["extractor.cpp"],
  deps = [
    ":cameras",
    ":files",
    ":timing",
    "//third_party:opencv",
    "//third_party:openpose",
  ],
)

cc_library(
  name = "files",
  hdrs = ["files.h"],
  srcs = ["files.cpp"],
)

cc_binary(
  name = "recorder",
  srcs = ["recorder.cpp"],
  deps = [
    ":cameras",
    ":files",
    ":timing",
    "//third_party:opencv",
  ],
)

cc_library(
  name = "timing",
  hdrs = ["timing.h"],
  srcs = ["timing.cpp"],
)

cc_binary(
  name = "track",
  srcs = ["track.cpp"],
  deps = ["//third_party:opencv"],
)
