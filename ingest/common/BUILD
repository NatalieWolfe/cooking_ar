load("@rules_cc//cc:defs.bzl", "cc_library")

package(
  default_visibility = ["//ingest:internal"],
)

cc_library(
  name = "base_service",
  hdrs = ["base_service.h"],
  deps = [":service_directory"],
)

cc_library(
  name = "events",
  hdrs = ["events.h"],
)

cc_library(
  name = "service_directory",
  hdrs = ["service_directory.h"],
)
