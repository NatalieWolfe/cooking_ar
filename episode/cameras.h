#pragma once

#include <filesystem>
#include <opencv2/core.hpp>

namespace episode {

struct CameraCalibration {
  struct Parameters {
    cv::Mat matrix;
    cv::Mat distortion;
    cv::Mat rotation;
    cv::Mat translation;
  };

  Parameters left;
  Parameters right;
  Parameters center;
};

CameraCalibration load_camera_calibration(
  const std::filesystem::path& calibration_path
);

}
