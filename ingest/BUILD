load("@rules_cc//cc:defs.bzl", "cc_binary")

package_group(
  name = "internal",
  packages = ["//ingest/..."],
)

cc_binary(
  name = "ingest",
  srcs = ["ingest.cpp"],
  deps = [
    "//ingest/common:service_directory",
    "//ingest/session:service",
    "//ingest/video:service",
  ],
)
