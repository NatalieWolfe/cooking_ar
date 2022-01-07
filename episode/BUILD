load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
  name = "project",
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
