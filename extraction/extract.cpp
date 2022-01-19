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
#include "extraction/pose2d.h"
#include "extraction/pose3d.h"
#include "extraction/pose_extractor.h"
#include "extraction/triangulation.h"
#include "lw/base/init.h"
#include "lw/flags/flags.h"

LW_FLAG(
  bool, reextract_2d, false,
  "When enabled, 2d pose extraction will happen even if a 2d pose file already "
  "exists for a frame."
);

namespace {

using ::episode::CameraCalibration;
using ::episode::CameraDirectory;
using ::episode::FrameRange;
using ::episode::Project;
using ::extraction::Pose2d;
using ::extraction::Pose3d;
using ::extraction::PoseExtractor;
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
 * @brief Extracts 2d poses for every image contained in `dir`.
 *
 * Use `--reextract_2d` to force re-extraction for frames that already have pose
 * files on disk.
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
    if (lw::flags::reextract_2d || !std::filesystem::exists(pose_path)) {
      // Perform extraction and save the results to disk.
      std::vector<Pose2d> poses = extractor.get(frame_path);
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

/**
 * @brief Reprojects the poses in each frame recorded for the given camera.
 */
void triangulate_all_poses(const Project& project, const CameraDirectory& cam) {
  FrameRange left_frames{cam.left_recording};
  FrameRange right_frames{cam.right_recording};
  const std::size_t frame_count = left_frames.size(); // Expensive.
  if (frame_count != right_frames.size()) {
    throw lw::FailedPrecondition()
      << "Camera has unequal frame counts: " << cam.path;
  }

  CameraCalibration calibration =
    episode::load_camera_calibration(cam.calibration_file);
  cli::Progress reporter;
  std::size_t skipped = 0;
  for (std::size_t i = 0; i < frame_count; ++i) {
    std::vector<Pose2d> left_poses = extraction::read_poses2d(
      project.pose_path_for_frame(left_frames[i])
    );
    std::vector<Pose2d> right_poses = extraction::read_poses2d(
      project.pose_path_for_frame(right_frames[i])
    );
    if (left_poses.size() != right_poses.size()) {
      ++skipped;
      continue;
    }
    std::vector<Pose3d> poses;
    for (std::size_t pose_idx = 0; pose_idx < left_poses.size(); ++pose_idx) {
      poses.push_back(
        triangulate_pose(
          calibration,
          left_poses[pose_idx],
          right_poses[pose_idx])
      );
    }
    extraction::write_poses(
      project.pose3d_path_for_frame(left_frames[i]),
      poses
    );
    reporter.stream() << "3d reconstruction: " << i << " : " << frame_count;
    if (skipped) {
      reporter.stream() << " (" << skipped << " skipped)";
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
    triangulate_all_poses(project, cam);
  }

  return 0;
}
