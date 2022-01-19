#pragma once

#include <opencv2/core.hpp>
#include <span>
#include <vector>

#include "episode/cameras.h"
#include "extraction/pose2d.h"
#include "extraction/pose3d.h"

namespace extraction {

/**
 * @brief Reprojects the points from the poses as a 3D pose.
 */
class Triangulator {
public:
  Triangulator(const episode::CameraCalibration& calibration);

  Pose3d operator()(
    const Pose2d& left_pose,
    const Pose2d& right_pose
  ) const;

private:
  std::vector<Pose3d::Point> _triangulate(
    std::span<const Pose2d::Point> left_points,
    std::span<const Pose2d::Point> right_points
  ) const;

  Pose3d::Point _triangulate(
    const Pose2d::Point& left_point,
    const Pose2d::Point& right_point
  ) const;

  cv::Mat _cam_l;
  cv::Mat _cam_r;
  cv::Mat _transform_l;
  cv::Mat _transform_r;
  cv::Mat _matrix_l;
  cv::Mat _matrix_r;
  cv::Mat _distortion_l;
  cv::Mat _distortion_r;
};

}
