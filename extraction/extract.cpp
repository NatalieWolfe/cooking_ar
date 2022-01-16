#include <algorithm>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

#include "episode/frames.h"
#include "episode/project.h"
#include "extraction/pose2d.h"
#include "extraction/pose_extractor.h"
#include "lw/base/init.h"

namespace {

using ::episode::CameraDirectory;
using ::episode::FrameRange;
using ::episode::Project;
using ::extraction::Pose2d;
using ::extraction::PoseExtractor;
using ::std::chrono::duration;
using ::std::chrono::duration_cast;
using ::std::chrono::milliseconds;
using ::std::chrono::steady_clock;
using ::std::chrono::time_point;
using ::std::filesystem::path;

std::vector<CameraDirectory> list_cameras(const Project& project) {
  std::vector<CameraDirectory> cameras;
  for (const std::string& session : project.sessions()) {
    std::vector<CameraDirectory> session_cams = project.cameras(session);
    std::move(
      session_cams.begin(),
      session_cams.end(),
      std::back_inserter(cameras)
    );
  }
  return cameras;
}

void extract_poses(
  const Project& project,
  PoseExtractor& extractor,
  const path& dir
) {
  FrameRange frames{dir};
  std::string last_message;
  const std::size_t total_frames = frames.size();
  std::size_t count = 0;
  time_point start = steady_clock::now();
  for (const path& frame_path : frames) {
    // Perform extraction and save the results to disk.
    std::vector<Pose2d> poses = extractor.get(frame_path);
    extraction::write_poses(project.pose_path_for_frame(frame_path), poses);

    // Update the progress report.
    for (std::size_t i = 0; i < last_message.size(); ++i) {
      std::cout << '\b';
    }
    milliseconds elapsed_ms =
      duration_cast<milliseconds>(steady_clock::now() - start);
    if (elapsed_ms.count() == 0) continue;
    double fps = ++count / static_cast<double>(elapsed_ms.count()) * 1000.0;
    std::stringstream out_buffer;
    out_buffer
      << count << " of " << total_frames << " @ " << std::setprecision(2)
      << fps << " fps";
    last_message = out_buffer.str();
    std::cout << last_message << std::flush;
  }
}

}

int main(int argc, const char** argv) {
  if (!lw::init(&argc, argv)) return -1;
  if (argc != 2) {
    std::cerr << "Missing path to project directory." << std::endl;
    return -2;
  }

  Project project = Project::open(argv[1]);
  const std::vector<CameraDirectory>& cameras = list_cameras(project);
  PoseExtractor extractor;
  for (const CameraDirectory& cam : cameras) {
    std::cout << cam.right_recording << ": ";
    extract_poses(project, extractor, cam.right_recording);
    std::cout << std::endl << cam.left_recording << ": ";
    extract_poses(project, extractor, cam.left_recording);
    std::cout << std::endl;
  }

  return 0;
}
