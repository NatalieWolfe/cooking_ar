load("@rules_cc//cc:defs.bzl", "cc_library")

cc_library(
  name = "service",
  hdrs = ["service.h"],
  srcs = ["service.cpp"],
  visibility = ["//ingest:internal"],
  deps = [
    "//ingest/common:base_service",
    "//ingest/common:events",
    "//ingest/common:service_directory",
    "//ingest/proto:session_proto_cc",
    "//ingest/proto:video_proto_cc",
    "//ingest/session:service",
  ],
)
