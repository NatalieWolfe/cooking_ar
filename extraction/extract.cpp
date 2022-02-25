#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "cli/progress.h"
#include "episode/cameras.h"
#include "episode/frames.h"
#include "episode/project.h"
#include "extraction/pose3d.h"
#include "extraction/pose_extractor.h"
#include "extraction/triangulation.h"
#include "lw/base/init.h"
#include "lw/flags/flags.h"

LW_FLAG(
  bool, reextract_images, false,
  "When enabled, pose extraction from images will happen even if a pose file "
  "already exists for a frame."
);

namespace {

using ::episode::CameraCalibration;
using ::episode::CameraDirectory;
using ::episode::FrameRange;
using ::episode::Project;
using ::extraction::Pose2d;
using ::extraction::Pose3d;
using ::extraction::PoseExtractor;
using ::extraction::Triangulator;
using ::std::chrono::duration;
using ::std::chrono::duration_cast;
using ::std::chrono::milliseconds;
using ::std::chrono::steady_clock;
using ::std::chrono::time_point;
using ::std::filesystem::path;

/**
 * @brief Lists the camera directories for every camera in every session in the
 * given project.
 */
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

/**
 * @brief Extracts single-reference poses for every image contained in `dir`.
 *
 * Use `--reextract-images` to force re-extraction for frames that already have
 * pose files on disk.
 */
void extract_poses(
  const Project& project,
  PoseExtractor& extractor,
  const path& dir
) {
  cli::Progress reporter;
  FrameRange frames{dir};
  std::string last_message;
  const std::size_t total_frames = frames.size();
  std::size_t count = 0;
  std::size_t skipped_count = 0;
  time_point start = steady_clock::now();
  for (const path& frame_path : frames) {
    path pose_path = project.pose_path_for_frame(frame_path);
    if (lw::flags::reextract_images || !std::filesystem::exists(pose_path)) {
      // Perform extraction and save the results to disk.
      std::vector<Pose3d> poses = extractor.get(frame_path);
      extraction::write_poses(pose_path, poses);
    } else {
      ++skipped_count;
    }

    // Update the progress report.
    milliseconds elapsed_ms =
      duration_cast<milliseconds>(steady_clock::now() - start);
    if (elapsed_ms.count() == 0) continue;
    double fps = ++count / static_cast<double>(elapsed_ms.count()) * 1000.0;
    reporter.stream()
      << count << " of " << total_frames << " @ " << std::setprecision(2)
      << fps << " fps";
    if (skipped_count) {
      reporter.stream() << " (skipped " << skipped_count << ")";
    }
    reporter.print();
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
    std::cout << cam.left_recording << ": ";
    extract_poses(project, extractor, cam.left_recording);
  }

  return 0;
}
