#include "src/cameras.h"

#include <cstring>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <linux/videodev2.h>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <regex>
#include <string>
#include <sys/ioctl.h>
#include <vector>

namespace {
namespace fs = std::filesystem;

void write(cv::FileStorage& file, const CameraDevice& device) {
  file.write("device_path", device.device_path.string());
  file.write("camera_id", device.camera_id);
  file.write("name", device.name);
}

void write(cv::FileStorage& file, const CameraParameters& parameters) {
  file.startWriteStruct("device", cv::FileNode::MAP, "CameraDevice");
  write(file, parameters.device);
  file.endWriteStruct();
  file.write("matrix", parameters.matrix);
  file.write("distortion", parameters.distortion);
  file.write("rotation", parameters.rotation);
  file.write("translation", parameters.translation);
}

CameraDevice read_device(const cv::FileNode& file) {
  return CameraDevice{
    .device_path = static_cast<std::string>(file["device_path"]),
    .camera_id = static_cast<int>(file["camera_id"]),
    .name = static_cast<std::string>(file["name"])
  };
}

CameraParameters read_parameters(const cv::FileNode& file) {
  return CameraParameters{
    .device{read_device(file["device"])},
    .matrix{file["matrix"].mat()},
    .distortion{file["distortion"].mat()},
    .rotation{file["rotation"].mat()},
    .translation{file["translation"].mat()}
  };
}

}

std::vector<CameraDevice> get_camera_devices() {
  std::vector<CameraDevice> cameras;

  std::regex video_pattern{"/dev/video(\\d+)"};
  fs::path devices = "/dev";
  for (const fs::directory_entry& device : fs::directory_iterator(devices)) {
    const auto& device_path_str = device.path().string();
    std::smatch m;
    if (!std::regex_match(device_path_str, m, video_pattern)) continue;

    int device_fd = ::open(device_path_str.c_str(), O_RDWR);
    if (device_fd < 0) throw std::runtime_error("Failed to open device.");

    v4l2_capability capability;
    if (::ioctl(device_fd, VIDIOC_QUERYCAP, &capability) < 0) {
      throw std::runtime_error("Failed to query device capabilities.");
    }
    if ((capability.device_caps & V4L2_CAP_VIDEO_CAPTURE) == 0) continue;

    cameras.push_back(CameraDevice{
      .device_path = device.path(),
      .camera_id = std::stoi(m[1].str(), 0, 10),
      .name{reinterpret_cast<const char*>(capability.card)}
    });
  }

  cameras.shrink_to_fit();
  return cameras;
}

void save_camera_parameters(
  const CameraParameters& parameters,
  const std::filesystem::path& filename
) {
  cv::FileStorage file{
    filename.string(),
    cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML
  };
  write(file, parameters);
  file.release();
}

CameraParameters load_camera_parameters(const std::filesystem::path& filename) {
  cv::FileStorage file{
    filename.string(),
    cv::FileStorage::READ | cv::FileStorage::FORMAT_YAML
  };
  return read_parameters(file.root());
}

Rectifier::Rectifier(CameraParameters parameters, cv::Size image_size):
  _parameters{std::move(parameters)}
{
  _optimal_matrix = cv::getOptimalNewCameraMatrix(
    _parameters.matrix,
    _parameters.distortion,
    image_size,
    0.0,
    image_size
  );
  cv::initUndistortRectifyMap(
    _parameters.matrix,
    _parameters.distortion,
    cv::noArray(),
    _optimal_matrix,
    image_size,
    CV_16SC2,
    _undistorted_map_1,
    _undistorted_map_2
  );
}

cv::Mat Rectifier::rectify(const cv::Mat& image) const {
  cv::Mat rectified;
  cv::remap(
    image,
    rectified,
    _undistorted_map_1,
    _undistorted_map_2,
    cv::INTER_CUBIC
  );
  return rectified;
}
