load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_proto_library")

package(
  default_visibility = ["//ingest:ingernal"],
)

proto_library(
  name = "camera_proto",
  srcs = ["camera.proto"],
)

cc_proto_library(
  name = "camera_proto_cc",
  deps = [":camera_proto"],
)

proto_library(
  name = "session_proto",
  srcs = ["session.proto"],
  deps = [":camera_proto"],
)

cc_proto_library(
  name = "session_proto_cc",
  deps = [":session_proto"],
)
