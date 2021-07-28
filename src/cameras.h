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

// TODO: Add camera resolution to calibration name and parameters.
// TODO: Add coordinate space id for rotation and translation.
struct CameraParameters {
  CameraDevice device;
  cv::Mat matrix;
  cv::Mat distortion;
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

class Rectifier {
public:
  Rectifier() = default;
  ~Rectifier() = default;
  Rectifier(const Rectifier&) = default;
  Rectifier(Rectifier&&) = default;
  Rectifier& operator=(const Rectifier&) = default;
  Rectifier& operator=(Rectifier&&) = default;

  explicit Rectifier(CameraParameters parameters, cv::Size image_size);

  cv::Mat rectify(const cv::Mat& image) const;
  cv::Mat operator()(const cv::Mat& image) const { return rectify(image); }

private:
  CameraParameters _parameters;
  cv::Mat _undistorted_map_1;
  cv::Mat _undistorted_map_2;
  cv::Mat _optimal_matrix;
};
