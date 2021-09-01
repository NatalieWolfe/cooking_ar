load("@rules_python//python:defs.bzl", "py_binary")
load("@pip_monitor//:requirements.bzl", "requirement")

filegroup(
  name = "html_templates",
  srcs = ["index.html"],
)

py_binary(
  name = "monitor",
  srcs = ["monitor.py"],
  data = [":html_templates"],
  deps = [
    requirement("Flask"),
    requirement("jinja2"),
  ],
)
