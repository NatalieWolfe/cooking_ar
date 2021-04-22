#include <cmath>
#include <filesystem>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <vector>

#include "src/cameras.h"
#include "src/files.h"
#include "src/tracking.h"


// Possible fixes:
// - Recalibrate cameras @ 640x480
// - Run remapping through calib3d: stereoRectify, undistortRectifyMap, remap, StereoBM


/**
 * Computes a unit vector in world-space pointing from the camera through the
 * given point on the image plane.
 */
cv::Mat to_ray(const CameraParameters& params, const Point& point) {
  // TODO: Switch to undistorted matrix.
  double f_x = params.matrix.at<double>(0, 0);
  double c_x = params.matrix.at<double>(0, 2);
  double f_y = params.matrix.at<double>(1, 1);
  double c_y = params.matrix.at<double>(1, 2);

  // Ray from camera origin pointing at pixel on image plane.
  double p_x = (point.x - c_x) / f_x;
  double p_y = (point.y - c_y) / f_y;
  double p_z = 1;
  double magnitude = std::sqrt((p_x * p_x) + (p_y * p_y) + (p_z * p_z));
  p_x /= magnitude;
  p_y /= magnitude;
  p_z /= magnitude;
  cv::Vec3d cam_ray{p_x, p_y, p_z};

  // Convert the rvec into a 3x3 matrix.
  cv::Mat transform;
  cv::Rodrigues(params.rotation, transform);

  cv::Mat origin_ray = transform * cam_ray;
  return origin_ray;
}

Point3d project_point(
  const CameraParameters& params_1,
  const Point& point_1,
  const CameraParameters& params_2,
  const Point& point_2
) {
  // Midpoint of line tracing the minimum distance between these two rays.
  // See this answer for the equations followed here and the origin of the
  // variable naming.
  // https://math.stackexchange.com/a/1037202/918090
  cv::Mat cam_1 = params_1.translation; // a
  cv::Mat cam_2 = params_2.translation; // c
  cv::Mat ray_1 = to_ray(params_1, point_1); // b
  cv::Mat ray_2 = to_ray(params_2, point_2); // d

  double b_dot_d = ray_1.dot(ray_2);
  double a_dot_d = cam_1.dot(ray_2);
  double b_dot_c = ray_1.dot(cam_2);
  double c_dot_d = cam_2.dot(ray_2);
  double a_dot_b = cam_1.dot(ray_1);

  double s = (
    ((b_dot_d * (a_dot_d - b_dot_c)) - (a_dot_d * c_dot_d)) /
    ((b_dot_d * b_dot_d) - 1)
  );
  double t = (
    ((b_dot_d * (c_dot_d - (a_dot_d)) - (b_dot_c * a_dot_b))) /
    ((b_dot_d * b_dot_d) - 1)
  );

  cv::Mat midpoint = (cam_1 + cam_2 + (t * ray_1) + (s * ray_2)) / 2;
  return Point3d{
    .point_id = point_1.point_id,
    .x = midpoint.at<double>(0, 0),
    .y = midpoint.at<double>(1, 0),
    .z = midpoint.at<double>(2, 0),
    .confidence = point_1.confidence * point_2.confidence
  };
}

// std::vector<Point3d> project_camera_points(
//   const CameraParameters& params,
//   const std::vector<Point> points
// ) {
//   std::vector<Point3d> projected;
//   std::vector<cv::Point3d> cv_projected;
//   std::vector<cv::Point2d> cv_points;
//   projected.reserve(points.size());
//   cv_projected.reserve(points.size());
//   cv_points.reserve(points.size());
//   for (const Point& point : points) cv_points.push_back(point.coords);
//   cv::projectPoints(
//     cv_points,
//     params.rotation,
//     params.translation,
//     params.matrix,
//     params.distortion,
//     cv_projected
//   );
// }

std::vector<Point3d> project_points(
  const CameraParameters& params_1,
  const std::vector<Point>& points_1,
  const CameraParameters& params_2,
  const std::vector<Point>& points_2
) {
  std::vector<Point3d> points;
  for (std::size_t i = 0; i < points_1.size(); ++i) {
    points.push_back(
      project_point(params_1, points_1[i], params_2, points_2[i])
    );
  }
  return points;
}

int main() {
  auto recordings_iterator =
    std::filesystem::directory_iterator{get_recordings_directory_path()};
  std::vector<std::filesystem::path> camera_directories;
  for (const auto& cam_dir : recordings_iterator) {
    camera_directories.push_back(cam_dir.path());
  }
  // TODO: Add support for more than 2 cameras.
  std::filesystem::path camera_1 = camera_directories[0];
  std::filesystem::path camera_2 = camera_directories[1];

  auto cam_1_parameters =
    load_camera_parameters(get_calibration_path(camera_1.stem().string()));
  auto cam_2_parameters =
    load_camera_parameters(get_calibration_path(camera_2.stem().string()));
  std::filesystem::path cam_2_dir =
    get_recordings_path(cam_2_parameters.device.camera_id);

  auto frame_iterator = std::filesystem::directory_iterator{camera_1};
  for (const auto& entry : frame_iterator) {
    if (entry.path().extension() != ".yml") continue;
    std::vector<Person> cam_1_frame = load_people(entry.path());
    std::vector<Person> cam_2_frame = load_people(cam_2_dir / entry.path().filename());

    std::vector<Person3d> frame_3d;
    for (std::size_t i = 0; i < cam_1_frame.size() && i < cam_2_frame.size(); ++i) {
      Person cam_1 = cam_1_frame[i];
      Person cam_2 = cam_2_frame[i];
      frame_3d.push_back(Person3d{
        .person_id = cam_1.person_id,
        .body = project_points(
          cam_1_parameters, cam_1.body,
          cam_2_parameters, cam_2.body
        ),
        .face = project_points(
          cam_1_parameters, cam_1.face,
          cam_2_parameters, cam_2.face
        ),
        .right_paw = project_points(
          cam_1_parameters, cam_1.right_paw,
          cam_2_parameters, cam_2.right_paw
        ),
        .left_paw = project_points(
          cam_1_parameters, cam_1.left_paw,
          cam_2_parameters, cam_2.left_paw
        )
      });
    }
    save_people_3d(frame_3d, get_animation_directory_path() / entry.path().filename());
  }
}
