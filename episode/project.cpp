#include "episode/project.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iomanip>
#include <string_view>
#include <sstream>
#include <vector>

#include "lw/err/canonical.h"

namespace episode {
namespace {

using ::std::filesystem::create_directories;
using ::std::filesystem::directory_iterator;
using ::std::filesystem::is_directory;
using ::std::filesystem::path;
using ::std::chrono::system_clock;

const path SESSION_DIR = "sessions";
const path CAMERA_DIR = "cameras";
const path LEFT_RECORDING_DIR = "recordings/left";
const path RIGHT_RECORDING_DIR = "recordings/right";
const path CALIBRATION_FILE = "calibration.json";
const path POSE_SUFFIX = "_pose.json";

std::string make_session_id() {
  std::stringstream id;
  std::time_t now = system_clock::to_time_t(system_clock::now());
  id << std::put_time(std::gmtime(&now), "%FT%T");
  return id.str();
}

CameraDirectory make_camera_directory(
  const path& session_path,
  std::string_view name
) {
  path cam_path = session_path / CAMERA_DIR / name;
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

Project Project::open(path dir) {
  if (!std::filesystem::exists(dir)) {
    throw lw::NotFound() << "No project found at " << dir;
  }
  return Project(std::move(dir), "");
}

Project Project::new_session(path dir) {
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

path Project::session_directory(std::string_view session) const {
  path dir = directory() / SESSION_DIR / session;
  if (!is_directory(dir)) {
    throw lw::NotFound() << "No directory for session " << session;
  }
  return dir;
}

path Project::session_directory() const {
  if (_session_id.empty()) {
    throw lw::FailedPrecondition() << "No active session.";
  }
  return session_directory(_session_id);
}

bool Project::has_session(std::string_view session) const {
  return is_directory(directory() / SESSION_DIR / session);
}

std::vector<std::string> Project::sessions() const {
  std::vector<std::string> session_ids;
  path sessions_dir = directory() / SESSION_DIR;
  for (const auto& entry : directory_iterator{sessions_dir}) {
    if (entry.is_directory()) {
      session_ids.push_back(entry.path().filename());
    }
  }
  std::sort(session_ids.begin(), session_ids.end());
  return session_ids;
}

CameraDirectory Project::add_camera(std::string_view name) {
  if (_session_id.empty()) {
    throw lw::FailedPrecondition() << "No active session to add camera.";
  }

  CameraDirectory cam = make_camera_directory(session_directory(), name);
  create_directories(cam.left_recording);
  create_directories(cam.right_recording);
  return cam;
}

bool Project::has_camera(
  std::string_view name,
  std::string_view session
) const {
  // TODO(alaina): Perform a more thorough existance check.
  return is_directory(session_directory(session) / CAMERA_DIR / name);
}

bool Project::has_camera(std::string_view name) const {
  if (_session_id.empty()) {
    throw lw::FailedPrecondition() << "No active session.";
  }
  return has_camera(name, _session_id);
}

CameraDirectory Project::camera(
  std::string_view name,
  std::string_view session
) const {
  CameraDirectory cam_dir =
    make_camera_directory(session_directory(session), name);
  if (!is_directory(cam_dir.path)) {
    throw lw::NotFound()
      << "Session " << session << " does not have a camera named " << name;
  }
  return cam_dir;
}

CameraDirectory Project::camera(std::string_view name) const {
  if (_session_id.empty()) {
    throw lw::FailedPrecondition() << "No active session.";
  }
  return camera(name, _session_id);
}

std::vector<CameraDirectory> Project::cameras(std::string_view session) const {
  const path& session_path = session_directory(session);
  std::vector<CameraDirectory> cams;
  for (const auto& entry : directory_iterator{session_path / CAMERA_DIR}) {
    if (entry.is_directory()) {
      cams.push_back(
        make_camera_directory(session_path, entry.path().filename().string())
      );
    }
  }
  return cams;
}

std::vector<CameraDirectory> Project::cameras() const {
  if (_session_id.empty()) {
    throw lw::FailedPrecondition() << "No active session.";
  }
  return cameras(_session_id);
}

path Project::pose_path_for_frame(const path& frame_path) const {
  path pose_path = frame_path;
  pose_path.replace_extension() += POSE_SUFFIX;
  return pose_path;
}

}
