load("@rules_cc//cc:defs.bzl", "cc_binary")

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

cc_binary(
  name = "recorder",
  srcs = ["recorder.cpp"],
  linkopts = [
    "-L/usr/local/lib",
    "-pthread",
    "-lopencv_world",
  ],
)
