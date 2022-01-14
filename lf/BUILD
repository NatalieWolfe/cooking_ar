load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
  name = "queue",
  visibility = ["//visibility:public"],
  hdrs = ["queue.h"],
  srcs = ["queue.cpp"],
)

cc_test(
  name = "queue_test",
  srcs = ["queue_test.cpp"],
  deps = [
    ":queue",
    "@gtest//:gtest_main",
  ],
)
