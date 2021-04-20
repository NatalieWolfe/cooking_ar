#include "src/files.h"

#include <filesystem>
#include <pwd.h>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>

namespace {
namespace fs = std::filesystem;
fs::path get_home_path() {
  const char* home = ::getenv("HOME");
  if (home == nullptr) home = ::getpwuid(::getuid())->pw_dir;
  return home;
}

fs::path make_path(fs::path path) {
  fs::create_directories(path);
  return std::move(path);
}
}

const fs::path& get_output_root_path() {
  static const fs::path root_path = make_path(get_home_path() / "cooking_ar");
  return root_path;
}

const fs::path& get_recordings_directory_path() {
  static const fs::path recordings_path =
    make_path(get_output_root_path() / "recordings");
  return recordings_path;
}

const fs::path& get_calibration_directory_path() {
  static const fs::path calibration_root =
    make_path(get_output_root_path() / "calibration");
  return calibration_root;
}

fs::path get_recordings_path(int camera_id) {
  return make_path(get_recordings_directory_path() / std::to_string(camera_id));
}

fs::path get_calibration_path(int camera_id) {
  return get_calibration_path(std::to_string(camera_id));
}

fs::path get_calibration_path(std::string_view camera_name) {
  fs::path path = get_calibration_directory_path() / camera_name;
  path.replace_extension(".yml");
  return path;
}
