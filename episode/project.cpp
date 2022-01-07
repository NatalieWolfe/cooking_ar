#include "episode/project.h"

#include <filesystem>
#include <string_view>

namespace episode {
namespace {

using std::filesystem::create_directories;
using std::filesystem::path;

const path CAMERA_DIR = "cameras";
const path LEFT_RECORDING_DIR = "recordings/left";
const path RIGHT_RECORDING_DIR = "recordings/right";
const path CALIBRATION_FILE = "calibration.json";

}

Project Project::open(path dir) {
  create_directories(dir);
  create_directories(dir / CAMERA_DIR);
  return Project(std::move(dir));
}

void Project::destroy(const path& dir) {
  // TODO(alaina): Confirm the given path references a project first?
  std::filesystem::remove_all(dir);
}

Project::Project(path dir): _root{std::move(dir)} {
  _name = _root.filename();
  if (_name.empty()) {
    // TODO(alaina): Test that this correctly handles the trailing slash case.
    // "/foo/bar" and "/foo/bar/" should both result in the project name "bar".
    _name = _root.parent_path().filename();
  }
}

CameraDirectory Project::add_camera(std::string_view name) {
  CameraDirectory cam = camera(name);
  create_directories(cam.left_recording);
  create_directories(cam.right_recording);
  return cam;
}

bool Project::has_camera(std::string_view name) const {
  // TODO(alaina): Perform a more thorough existance check.
  return std::filesystem::exists(_root / CAMERA_DIR / name);
}

CameraDirectory Project::camera(std::string_view name) const {
  path cam_path = _root / CAMERA_DIR / name;
  CameraDirectory cam{
    .name = std::string{name},
    .path = cam_path,
    .left_recording = cam_path / LEFT_RECORDING_DIR,
    .right_recording = cam_path / RIGHT_RECORDING_DIR,
    .calibration_file = cam_path / CALIBRATION_FILE
  };
  return cam;
}

}
