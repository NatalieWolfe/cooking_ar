#include "extraction/triangulation.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <span>
#include <vector>

#include "episode/cameras.h"
#include "extraction/pose2d.h"
#include "extraction/pose3d.h"

namespace extraction {
namespace {

using ::episode::CameraCalibration;

cv::Mat cam_extrinsic_matrix(const CameraCalibration::Parameters& params) {
  // Move camera's position into world space.
  cv::Mat translation = cv::Mat::zeros(1, 3, CV_64F);
  translation.at<double>(0, 0) = params.translation.at<double>(0, 0);
  translation.at<double>(0, 1) = params.translation.at<double>(1, 0);
  translation.at<double>(0, 2) = params.translation.at<double>(2, 0);
  translation = translation * params.rotation;

  // Compose transformation matrix.
  cv::Mat transformation = cv::Mat::zeros(3, 4, CV_32F);
  transformation.at<float>(0, 0) =
    static_cast<float>(params.rotation.at<double>(0, 0));
  transformation.at<float>(0, 1) =
    static_cast<float>(params.rotation.at<double>(0, 1));
  transformation.at<float>(0, 2) =
    static_cast<float>(params.rotation.at<double>(0, 2));
  transformation.at<float>(1, 0) =
    static_cast<float>(params.rotation.at<double>(1, 0));
  transformation.at<float>(1, 1) =
    static_cast<float>(params.rotation.at<double>(1, 1));
  transformation.at<float>(1, 2) =
    static_cast<float>(params.rotation.at<double>(1, 2));
  transformation.at<float>(2, 0) =
    static_cast<float>(params.rotation.at<double>(2, 0));
  transformation.at<float>(2, 1) =
    static_cast<float>(params.rotation.at<double>(2, 1));
  transformation.at<float>(2, 2) =
    static_cast<float>(params.rotation.at<double>(2, 2));
  transformation.at<float>(0, 3) =
    -static_cast<float>(translation.at<double>(0, 0));
  transformation.at<float>(1, 3) =
    -static_cast<float>(translation.at<double>(0, 1));
  transformation.at<float>(2, 3) =
    -static_cast<float>(translation.at<double>(0, 2));

  return transformation;
}

cv::Mat cam_trans_to_world(const CameraCalibration::Parameters& params) {
  cv::Mat extrinsics = cam_extrinsic_matrix(params);
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
  const Pose2d::Point& point,
  const cv::Mat& matrix,
  const cv::Mat& distortion,
  const cv::Mat& transform
) {
  // TODO: Switch to undistorted matrix.
  cv::Mat in_points = cv::Mat::zeros(2, 1, CV_32F);
  std::vector<cv::Point2f> out_points;
  in_points.at<float>(0, 0) = point.x;
  in_points.at<float>(1, 0) = point.y;
  cv::undistortPoints(in_points, out_points, matrix, distortion);

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

}

Triangulator::Triangulator(const CameraCalibration& calibration):
  _cam_l{cam_trans_to_world(calibration.left)},
  _cam_r{cam_trans_to_world(calibration.right)},
  _transform_l{cam_extrinsic_matrix(calibration.left)},
  _transform_r{cam_extrinsic_matrix(calibration.right)},
  _matrix_l{calibration.left.matrix},
  _matrix_r{calibration.right.matrix},
  _distortion_l{calibration.left.distortion},
  _distortion_r{calibration.right.distortion}
{}

Pose3d Triangulator::operator()(
  const Pose2d& left_pose,
  const Pose2d& right_pose
) const {
  return {
    .person_id = left_pose.person_id,
    .body = _triangulate(left_pose.body, right_pose.body),
    .face = _triangulate(left_pose.face, right_pose.face),
    .left_paw = _triangulate(left_pose.left_paw, right_pose.left_paw),
    .right_paw = _triangulate(left_pose.right_paw, right_pose.right_paw),
  };
}

std::vector<Pose3d::Point> Triangulator::_triangulate(
  std::span<const Pose2d::Point> left_points,
  std::span<const Pose2d::Point> right_points
) const {
  std::vector<Pose3d::Point> points;
  points.reserve(left_points.size());
  for (std::size_t i = 0; i < left_points.size(); ++i) {
    points.push_back(
      _triangulate(left_points[i], right_points[i])
    );
  }
  return points;
}

Pose3d::Point Triangulator::_triangulate(
  const Pose2d::Point& left_point,
  const Pose2d::Point& right_point
) const {
  // Midpoint of line tracing the minimum distance between these two rays.
  // See this answer for the equations followed here and the origin of the
  // variable naming.
  // https://math.stackexchange.com/a/1037202/918090
  //
  // _cam_l -> a
  // ray_l  -> b
  // _cam_r -> c
  // ray_r  -> d
  cv::Mat ray_l = to_ray(left_point, _matrix_l, _distortion_l, _transform_l);
  cv::Mat ray_r = to_ray(right_point, _matrix_r, _distortion_r, _transform_r);

  double b_dot_d = ray_l.dot(ray_r);
  double a_dot_d = _cam_l.dot(ray_r);
  double b_dot_c = ray_l.dot(_cam_r);
  double c_dot_d = _cam_r.dot(ray_r);
  double a_dot_b = _cam_l.dot(ray_l);

  double s = (
    ((b_dot_d * (a_dot_d - b_dot_c)) - (a_dot_d * c_dot_d)) /
    ((b_dot_d * b_dot_d) - 1)
  );
  double t = (
    ((b_dot_d * (c_dot_d - a_dot_d)) - (b_dot_c * a_dot_b)) /
    ((b_dot_d * b_dot_d) - 1)
  );

  cv::Mat midpoint = (_cam_l + _cam_r + (t * ray_l) + (s * ray_r)) / 2.235f;
  return {
    .point_id = left_point.point_id,
    .x = midpoint.at<float>(0, 0),
    .y = midpoint.at<float>(1, 0),
    .z = midpoint.at<float>(2, 0),
    .confidence = left_point.confidence * right_point.confidence
  };
}

}
