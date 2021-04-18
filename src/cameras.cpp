#include "src/cameras.h"

#include <cstring>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <linux/videodev2.h>
#include <regex>
#include <string>
#include <sys/ioctl.h>
#include <vector>

namespace fs = std::filesystem;

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
