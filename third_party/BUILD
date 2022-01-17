load("@rules_cc//cc:defs.bzl", "cc_library")
load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

cmake(
  name = "opencv",
  visibility = ["//visibility:public"],
  lib_source = "@opencv//:all",
  build_args = ["--parallel=28"],
  copts = [],
  cache_entries = {
    "CMAKE_CXX_FLAGS": "-Wno-error",
    "BUILD_JAVA": "Off",
    "BUILD_JPEG": "Off",
    "BUILD_PERF_TESTS": "Off",
    "BUILD_SHARED_LIBS": "Off",
    "BUILD_TESTS": "Off",
    "BUILD_opencv_apps": "Off",
    "BUILD_opencv_java_bindings_generator": "Off",
    "BUILD_opencv_js": "Off",
    "BUILD_opencv_js_bindings_generator": "Off",
    "BUILD_opencv_obj_bindings_generator": "Off",
    "BUILD_opencv_python_tests": "Off",
    "BUILD_opencv_world": "On",
    "OPENCV_EXTRA_MODULES_PATH": "$(execpath @opencv_contrib//:modules)",
    "WITH_GTK": "On",
    "WITH_ITT": "Off",
    "WITH_JASPER": "Off",
    "WITH_JPEG": "Off",
    "WITH_OPENEXR": "Off",
    "WITH_OPENJPEG": "Off",
    "WITH_TIFF": "Off",
    "WITH_WEBP": "Off",
  },
  build_data = [
    "@opencv_contrib//:modules",
  ],
  out_static_libs = [
    "libopencv_world.a",
  ],
  linkopts = [
    "-pthread",
    "-latk-1.0",
    "-lcairo",
    "-lcairo-gobject",
    "-ldl",
    "-lgdk-3",
    "-lgdk_pixbuf-2.0",
    "-lgio-2.0",
    "-lglib-2.0",
    "-lgobject-2.0",
    "-lgtk-3",
    "-lharfbuzz",
    "-lpangocairo-1.0",
    "-lpango-1.0",
    "-lpng",
    "-lz",
  ],
)

cc_library(
  name = "openpose",
  visibility = ["//visibility:public"],
  linkopts = [
    "-L/usr/local/lib",
    "-lopenpose",
  ],
)

# Must build and install https://github.com/luxonis/depthai-core separately.
cc_library(
  name = "depthai",
  visibility = ["//visibility:public"],
  linkopts = [
    "-L/usr/local/lib",
    "-ldepthai-core",
    "-ldepthai-opencv",
    "-ldepthai-resources",
    "-lusb-1.0",
  ],
  deps = [
    ":opencv",
    "@depthai_3rdparty//:tl",
    "@depthai_dependencies//:archive",
    "@depthai_dependencies//:bz2",
    "@depthai_dependencies//:lzma",
    "@depthai_dependencies//:nlohmann",
    "@depthai_dependencies//:nop",
    "@depthai_dependencies//:spdlog",
    "@depthai_dependencies//:xlink",
    "@depthai_dependencies//:zlib",
  ],
)

cmake(
  name = "json",
  visibility = ["//visibility:public"],
  lib_source = "@nlohmann_json//:all",
  cache_entries = {
    "JSON_BuildTests": "Off",
    "JSON_Install": "On",
  },
  out_headers_only = True,
)
