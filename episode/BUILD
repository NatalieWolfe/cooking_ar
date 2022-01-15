load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
  name = "frames",
  visibility = ["//visibility:public"],
  hdrs = ["frames.h"],
  srcs = ["frames.cpp"],
)

cc_test(
  name = "frames_test",
  srcs = ["frames_test.cpp"],
  data = ["//episode/testing/fake_frames"],
  deps = [
    ":frames",
    "@gtest//:gtest_main",
  ],
)

cc_library(
  name = "project",
  visibility = ["//visibility:public"],
  hdrs = ["project.h"],
  srcs = ["project.cpp"],
)

cc_test(
  name = "project_test",
  srcs = ["project_test.cpp"],
  deps = [
    ":project",
    "@gtest//:gtest_main",
  ],
)
