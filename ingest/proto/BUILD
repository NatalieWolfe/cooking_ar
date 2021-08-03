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
  name = "extractor_proto",
  srcs = ["extractor.proto"],
  deps = [":camera_proto"],
)

cc_proto_library(
  name = "extractor_proto_cc",
  deps = [":extractor_proto"],
)

proto_library(
  name = "frame_coordinator_proto",
  srcs = ["frame_coordinator.proto"],
  deps = [
    ":camera_proto",
    ":extractor_proto",
  ],
)

cc_proto_library(
  name = "frame_coordinator_proto_cc",
  deps = [":frame_coordinator_proto"],
)

proto_library(
  name = "pose_proto",
  srcs = ["pose.proto"],
)

cc_proto_library(
  name = "pose_proto_cc",
  deps = [":pose_proto"],
)

proto_library(
  name = "projector_proto",
  srcs = ["projector.proto"],
  deps = [":extractor_proto"],
)

cc_proto_library(
  name = "projector_proto_cc",
  deps = [":projector_proto"],
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

proto_library(
  name = "video_proto",
  srcs = ["video.proto"],
  deps = [":camera_proto"],
)

cc_proto_library(
  name = "video_proto_cc",
  deps = [":video_proto"],
)
