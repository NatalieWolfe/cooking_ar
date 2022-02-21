load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_library", "cc_test")

cc_binary(
  name = "extract",
  srcs = ["extract.cpp"],
  deps = [
    ":pose2d",
    ":pose3d",
    ":pose_extractor",
    ":triangulation",
    "@lw//lw/base",
    "//cli:progress",
    "//episode:cameras",
    "//episode:frames",
    "//episode:project",
  ]
)

cc_library(
  name = "pose2d",
  hdrs = ["pose2d.h"],
  srcs = ["pose2d.cpp"],
  deps = [
    "@lw//lw/err",
    "//third_party:json",
  ],
)

cc_library(
  name = "pose3d",
  hdrs = ["pose3d.h"],
  srcs = ["pose3d.cpp"],
  deps = [
    "@lw//lw/err",
    "//third_party:json",
  ],
)

cc_library(
  name = "pose_extractor",
  hdrs = ["pose_extractor.h"],
  srcs = ["pose_extractor.cpp"],
  deps = [
    ":pose2d",
    "@lw//lw/flags",
    "//third_party:opencv",
    "//third_party:openpose",
  ],
)

cc_library(
  name = "triangulation",
  hdrs = ["triangulation.h"],
  srcs = ["triangulation.cpp"],
  deps = [
    ":pose2d",
    ":pose3d",
    "//episode:cameras",
    "//third_party:opencv",
  ],
)

cc_test(
  name = "triangulation_test",
  srcs = ["triangulation_test.cpp"],
  data = ["//extraction/testing:pose_data"],
  deps = [
    ":pose2d",
    ":pose3d",
    ":triangulation",
    "@gtest//:gtest_main",
    "//third_party:opencv",
  ]
)

cc_binary(
  name = "visualize",
  srcs = ["visualize.cpp"],
  deps = [
    ":pose2d",
    ":pose3d",
    "@lw//lw/base",
    "@lw//lw/flags",
    "//cli:keys",
    "//episode:cameras",
    "//episode:frames",
    "//episode:project",
    "//third_party:opencv",
  ],
)

filegroup(
  name = "mediapipe_graph",
  data = ["mediapipe_graph.pbtxt"],
)

cc_binary(
  name = "mediapipe",
  srcs = ["mediapipe.cpp"],
  data = [
    ":mediapipe_graph",
    "//mediapipe/modules/face_detection:face_detection_short_range.tflite",
    "//mediapipe/modules/face_landmark:face_landmark.tflite",
    "//mediapipe/modules/hand_landmark:hand_landmark_full.tflite",
    "//mediapipe/modules/hand_landmark:handedness.txt",
    "//mediapipe/modules/holistic_landmark:hand_recrop.tflite",
    "//mediapipe/modules/pose_detection:pose_detection.tflite",
    "//mediapipe/modules/pose_landmark:pose_landmark_heavy.tflite",
  ],
  deps = [
    "@mediapipe//mediapipe/framework:calculator_framework",
    "@mediapipe//mediapipe/framework/formats:image_frame",
    "@mediapipe//mediapipe/framework/formats:image_frame_opencv",
    "@mediapipe//mediapipe/framework/formats:landmark_cc_proto",
    "@mediapipe//mediapipe/framework/port:file_helpers",
    "@mediapipe//mediapipe/framework/port:opencv_highgui",
    "@mediapipe//mediapipe/framework/port:opencv_video",
    "@mediapipe//mediapipe/framework/port:parse_text_proto",
    "@mediapipe//mediapipe/framework/port:status",
    "@mediapipe//mediapipe/graphs/holistic_tracking:holistic_tracking_cpu_graph_deps",
  ],
)
