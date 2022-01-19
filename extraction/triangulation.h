#pragma once

#include "episode/cameras.h"
#include "extraction/pose2d.h"
#include "extraction/pose3d.h"

namespace extraction {

/**
 * @brief Reprojects the points from the given poses as a 3D pose.
 */
Pose3d triangulate_pose(
  const episode::CameraCalibration& calibration,
  const Pose2d& left_pose,
  const Pose2d& right_pose
);

}
