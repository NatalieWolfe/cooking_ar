load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_library(
  name = "oakd_camera",
  srcs = ["oakd_camera.cpp"],
  hdrs = ["oakd_camera.h"],
  deps = [
    "//third_party:depthai",
  ],
)

cc_test(
  name = "oakd_camera_test",
  srcs = ["oakd_camera_test.cpp"],
  deps = [
    ":oakd_camera",
    "@gtest//:gtest_main",
  ],
)

cc_binary(
  name = "record",
  srcs = ["record.cpp"],
  deps = [
    ":oakd_camera",
    "//episode:project",
    "//lf:queue",
    "//third_party:depthai",
  ],
)
