#include "recording/monitor/camera_controller.h"

#include <filesystem>
#include <memory>
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <string_view>

namespace recording {

using ::std::filesystem::path;

std::unique_ptr<CameraController> CameraController::create(
  std::string_view camera_host,
  const path& base_directory
) {
  std::size_t id = std::hash<std::string_view>{}(camera_host);
  path frames_directory = base_directory / std::to_string(id) / "frames";
  std::filesystem::create_directories(frames_directory);
  return std::unique_ptr<CameraController>{new CameraController{
    camera_host,
    frames_directory
  }};
}

CameraController::~CameraController() {
  _continue = false;
  _stream_thread.join();
}

cv::Mat CameraController::frame() const {
  cv::Mat frame_copy;
  {
    std::scoped_lock lock{_frame_guard};
    _frame.copyTo(frame_copy);
  }
  return frame_copy;
}

void CameraController::_camera_thread_main() {
  while (_continue) {
    std::uint64_t frame_id = _saver.save_frame();
    path frame_path = _saver.frame_path(frame_id);
    if (_display) _read_frame_data(frame_path);
    if (!_record) std::filesystem::remove(frame_path);
  }
}

void CameraController::_read_frame_data(const path& frame_path) {
  std::scoped_lock lock{_frame_guard};
  _frame = cv::imread(frame_path.string());
}

}
