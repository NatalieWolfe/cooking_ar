#include "episode/project.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <string_view>
#include <sstream>

namespace episode {
namespace {

using ::std::filesystem::create_directories;
using ::std::filesystem::path;
using ::std::chrono::system_clock;

const path SESSION_DIR = "sessions";
const path CAMERA_DIR = "cameras";
const path LEFT_RECORDING_DIR = "recordings/left";
const path RIGHT_RECORDING_DIR = "recordings/right";
const path CALIBRATION_FILE = "calibration.json";

std::string make_session_id() {
  std::stringstream id;
  std::time_t now = system_clock::to_time_t(system_clock::now());
  id << std::put_time(std::gmtime(&now), "%FT%T");
  return id.str();
}

}

Project Project::open(path dir) {
  std::string session_id = make_session_id();
  create_directories(dir);
  create_directories(dir / SESSION_DIR / session_id / CAMERA_DIR);
  return Project(std::move(dir), std::move(session_id));
}

void Project::destroy(const path& dir) {
  // TODO(alaina): Confirm the given path references a project first?
  std::filesystem::remove_all(dir);
}

Project::Project(path dir, std::string session_id):
  _root{std::move(dir)},
  _session_id{std::move(session_id)}
{
  _name = _root.filename();
  if (_name.empty()) {
    // TODO(alaina): Test that this correctly handles the trailing slash case.
    // "/foo/bar" and "/foo/bar/" should both result in the project name "bar".
    _name = _root.parent_path().filename();
  }
}

std::filesystem::path Project::session_directory() const {
  return directory() / SESSION_DIR / _session_id;
}

CameraDirectory Project::add_camera(std::string_view name) {
  CameraDirectory cam = camera(name);
  create_directories(cam.left_recording);
  create_directories(cam.right_recording);
  return cam;
}

bool Project::has_camera(std::string_view name) const {
  // TODO(alaina): Perform a more thorough existance check.
  return std::filesystem::exists(session_directory() / CAMERA_DIR / name);
}

CameraDirectory Project::camera(std::string_view name) const {
  path cam_path = session_directory() / CAMERA_DIR / name;
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
