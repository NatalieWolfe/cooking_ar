#pragma once

#include <filesystem>
#include <string>
#include <opencv2/core.hpp>
#include <vector>

struct CameraDevice {
  std::filesystem::path device_path;
  int camera_id;
  std::string name;
};

std::vector<CameraDevice> get_camera_devices();
