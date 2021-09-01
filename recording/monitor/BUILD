load("@rules_python//python:defs.bzl", "py_binary")
load("@pip_monitor//:requirements.bzl", "requirement")

py_binary(
  name = "monitor",
  srcs = ["monitor.py"],
  deps = [
    requirement("Flask"),
  ],
)
