#include <algorithm>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <span>
#include <string>
#include <vector>

#include "cli/progress.h"
#include "episode/cameras.h"
#include "episode/frames.h"
#include "episode/project.h"
#include "extraction/pose2d.h"
#include "extraction/pose3d.h"
#include "extraction/pose_extractor.h"
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

template <typename T>
cv::Mat cam_extrinsic_matrix(const CameraCalibration::Parameters& params) {
  // Convert the rvec into a 3x3 matrix.
  cv::Mat rotation;
  cv::Rodrigues(params.rotation, rotation);

  // Move camera's position into world space.
  cv::Mat translation = cv::Mat::zeros(1, 3, CV_64F);
  translation.at<double>(0, 0) = params.translation.at<double>(0, 0);
  translation.at<double>(0, 1) = params.translation.at<double>(1, 0);
  translation.at<double>(0, 2) = params.translation.at<double>(2, 0);
  translation = translation * rotation;

  // Compose transformation matrix.
  cv::Mat transformation = cv::Mat::zeros(3, 4, CV_32F);
  transformation.at<T>(0, 0) = static_cast<T>(rotation.at<double>(0, 0));
  transformation.at<T>(0, 1) = static_cast<T>(rotation.at<double>(0, 1));
  transformation.at<T>(0, 2) = static_cast<T>(rotation.at<double>(0, 2));
  transformation.at<T>(1, 0) = static_cast<T>(rotation.at<double>(1, 0));
  transformation.at<T>(1, 1) = static_cast<T>(rotation.at<double>(1, 1));
  transformation.at<T>(1, 2) = static_cast<T>(rotation.at<double>(1, 2));
  transformation.at<T>(2, 0) = static_cast<T>(rotation.at<double>(2, 0));
  transformation.at<T>(2, 1) = static_cast<T>(rotation.at<double>(2, 1));
  transformation.at<T>(2, 2) = static_cast<T>(rotation.at<double>(2, 2));
  transformation.at<T>(0, 3) = -static_cast<T>(translation.at<double>(0, 0));
  transformation.at<T>(1, 3) = -static_cast<T>(translation.at<double>(0, 1));
  transformation.at<T>(2, 3) = -static_cast<T>(translation.at<double>(0, 2));

  return transformation;
}

cv::Mat cam_trans_to_world(const CameraCalibration::Parameters& params) {
  cv::Mat extrinsics = cam_extrinsic_matrix<float>(params);
  cv::Mat translation = cv::Mat::zeros(3, 1, CV_32F);
  translation.at<float>(0, 0) = extrinsics.at<float>(0, 3);
  translation.at<float>(1, 0) = extrinsics.at<float>(1, 3);
  translation.at<float>(2, 0) = extrinsics.at<float>(2, 3);
  return translation;
}

float calc_magnitude(float x, float y, float z) {
  return std::sqrt((x * x) + (y * y) + (z * z));
}

/**
 * Computes a unit vector in world-space pointing from the camera through the
 * given point on the image plane.
 */
cv::Mat to_ray(
  const CameraCalibration::Parameters& params,
  const Pose2d::Point& point
) {
  // TODO: Switch to undistorted matrix.
  cv::Mat in_points = cv::Mat::zeros(2, 1, CV_32F);
  std::vector<cv::Point2f> out_points;
  in_points.at<float>(0, 0) = point.x;
  in_points.at<float>(1, 0) = point.y;
  cv::undistortPoints(in_points, out_points, params.matrix, params.distortion);

  // Ray from camera origin pointing at pixel on image plane.
  float p_x = out_points[0].x;
  float p_y = out_points[0].y;
  float p_z = 1.0f;
  float magnitude = calc_magnitude(p_x, p_y, p_z);
  p_x /= magnitude;
  p_y /= magnitude;
  p_z /= magnitude;

  cv::Mat cam_ray = cv::Mat::zeros(1, 3, CV_32F);
  cam_ray.at<float>(0, 0) = p_x;
  cam_ray.at<float>(0, 1) = p_y;
  cam_ray.at<float>(0, 2) = p_z;

  // Convert the rvec into a 3x3 matrix.
  cv::Mat transform = cam_extrinsic_matrix<float>(params);

  cv::Mat origin_ray = cam_ray * transform;
  float x = origin_ray.at<float>(0, 0);
  float y = origin_ray.at<float>(0, 1);
  float z = origin_ray.at<float>(0, 2);
  magnitude = calc_magnitude(x, y, z);
  cam_ray = cv::Mat::zeros(3, 1, CV_32F);
  cam_ray.at<float>(0, 0) = x / magnitude;
  cam_ray.at<float>(1, 0) = y / magnitude;
  cam_ray.at<float>(2, 0) = z / magnitude;

  return cam_ray;
}

Pose3d::Point triangulate(
  const CameraCalibration& calibration,
  const Pose2d::Point& left_point,
  const Pose2d::Point& right_point
) {
  // Midpoint of line tracing the minimum distance between these two rays.
  // See this answer for the equations followed here and the origin of the
  // variable naming.
  // https://math.stackexchange.com/a/1037202/918090
  cv::Mat cam_l = cam_trans_to_world(calibration.left);   // a
  cv::Mat cam_r = cam_trans_to_world(calibration.right);  // c
  cv::Mat ray_l = to_ray(calibration.left, left_point);   // b
  cv::Mat ray_r = to_ray(calibration.right, right_point); // d

  double b_dot_d = ray_l.dot(ray_r);
  double a_dot_d = cam_l.dot(ray_r);
  double b_dot_c = ray_l.dot(cam_r);
  double c_dot_d = cam_r.dot(ray_r);
  double a_dot_b = cam_l.dot(ray_l);

  double s = (
    ((b_dot_d * (a_dot_d - b_dot_c)) - (a_dot_d * c_dot_d)) /
    ((b_dot_d * b_dot_d) - 1)
  );
  double t = (
    ((b_dot_d * (c_dot_d - a_dot_d)) - (b_dot_c * a_dot_b)) /
    ((b_dot_d * b_dot_d) - 1)
  );

  cv::Mat midpoint = (cam_l + cam_r + (t * ray_l) + (s * ray_r)) / 2.235f;
  return {
    .point_id = left_point.point_id,
    .x = midpoint.at<float>(0, 0),
    .y = midpoint.at<float>(1, 0),
    .z = midpoint.at<float>(2, 0),
    .confidence = left_point.confidence * right_point.confidence
  };
}

std::vector<Pose3d::Point> triangulate(
  const CameraCalibration& calibration,
  std::span<const Pose2d::Point> left_points,
  std::span<const Pose2d::Point> right_points
) {
  std::vector<Pose3d::Point> points;
  for (std::size_t i = 0; i < left_points.size(); ++i) {
    points.push_back(
      triangulate(calibration, left_points[i], right_points[i])
    );
  }
  return points;
}

/**
 * @brief Reprojects the points from the given poses as a 3D pose.
 */
Pose3d triangulate(
  const CameraCalibration& calibration,
  const Pose2d& left_pose,
  const Pose2d& right_pose
) {
  return {
    .person_id = left_pose.person_id,
    .body = triangulate(calibration, left_pose.body, right_pose.body),
    .face = triangulate(calibration, left_pose.face, right_pose.face),
    .left_paw =
      triangulate(calibration, left_pose.left_paw, right_pose.left_paw),
    .right_paw =
      triangulate(calibration, left_pose.right_paw, right_pose.right_paw),
  };
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
  for (std::size_t i = 0; i < frame_count; ++i) {
    std::vector<Pose2d> left_poses = extraction::read_poses2d(
      project.pose_path_for_frame(left_frames[i])
    );
    std::vector<Pose2d> right_poses = extraction::read_poses2d(
      project.pose_path_for_frame(right_frames[i])
    );
    if (left_poses.size() != right_poses.size()) {
      std::cerr
        << std::endl << "Skipping mis-matched frame: " << left_frames[i]
        << std::endl;
      continue;
      // throw lw::Internal()
      //   << "Frame has different number of poses captured (left: "
      //   << left_poses.size() << "; right: " << right_poses.size() << "): "
      //   << left_frames[i];
    }
    std::vector<Pose3d> poses;
    for (std::size_t pose_idx = 0; pose_idx < left_poses.size(); ++pose_idx) {
      poses.push_back(
        triangulate(calibration, left_poses[pose_idx], right_poses[pose_idx])
      );
    }
    extraction::write_poses(
      project.pose3d_path_for_frame(left_frames[i]),
      poses
    );
    reporter.stream() << "3d reconstruction: " << i << " : " << frame_count;
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
