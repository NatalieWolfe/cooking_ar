#pragma once

#include <filesystem>
#include <opencv2/core.hpp>
#include <string>
#include <vector>

struct CameraDevice {
  std::filesystem::path device_path;
  int camera_id;
  std::string name;
};

struct CameraParameters {
  CameraDevice device;
  cv::Mat matrix;
  cv::Mat distortion;
  cv::Mat undistorted_matrix;
  cv::Mat rotation;
  cv::Mat translation;
};

/**
 * Fetches the ID and paths of webcams plugged into the machine.
 */
std::vector<CameraDevice> get_camera_devices();

CameraParameters load_camera_parameters(const std::filesystem::path& filename);
void save_camera_parameters(
  const CameraParameters& parameters,
  const std::filesystem::path& filename
);
