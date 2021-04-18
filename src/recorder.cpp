#include <cstdlib>
#include <exception>
#include <fcntl.h>
#include <filesystem>
#include <ios>
#include <iostream>
#include <linux/v4l2-common.h>
#include <linux/videodev2.h>
#include <regex>
#include <sys/ioctl.h>
#include <vector>

#include <functional>
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <thread>

#include <chrono>

static std::size_t frame_counter = 0;
const std::filesystem::path recording_dir{"recording"};

class Camera {
private:
  enum class Action {
    NONE,
    EXIT,
    CAPTURE
  };

public:
  explicit Camera(int camera_id): _camera_id{camera_id} {
    _camera_input.open(_camera_id);
    _camera_input.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    _camera_input.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    _thread = std::thread{std::bind(&Camera::_thread_loop, this)};

    _save_path = recording_dir / std::to_string(_camera_id);
    std::filesystem::create_directories(_save_path);
  }

  ~Camera() {
    _trigger_action(Action::EXIT);
    _thread.join();
  }

  void capture() { _trigger_action(Action::CAPTURE); }
  cv::Mat last_capture() {
    cv::Mat image;
    {
      std::unique_lock<std::mutex> lock{_action_mutex};
      // _last_capture.copyTo(image);
      _last_frame.copyTo(image);
    }
    return image;
  }

private:
  void _thread_loop() {
    std::unique_lock<std::mutex> action_lock{_action_mutex};
    while (_action != Action::EXIT) {
      _action_flag.wait(
        action_lock,
        [this]() { return _action != Action::NONE; }
      );
      switch (_action) {
        case Action::NONE:
        case Action::EXIT: continue;

        case Action::CAPTURE: {
          _capture();
          break;
        }
      }
      _action = Action::NONE;
    }
  }

  void _trigger_action(Action action) {
    std::unique_lock<std::mutex> lock{_action_mutex};
    _action = action;
    _action_flag.notify_all();
  }

  void _capture() {
    _camera_input >> _last_frame;
    std::filesystem::path image_name =
      _save_path / (std::to_string(frame_counter) + ".png");
    if (!cv::imwrite(image_name.c_str(), _last_frame)) {
      throw std::runtime_error("Failed to save frame " + image_name.string());
    }
  }

  Action _action = Action::NONE;
  std::mutex _action_mutex;
  std::condition_variable _action_flag;

  int _camera_id;
  std::thread _thread;
  std::filesystem::path _save_path;

  cv::VideoCapture _camera_input;
  cv::Mat _last_frame;
};

std::vector<int> get_camera_ids() {
  std::vector<int> camera_ids;

  std::regex video_pattern{"/dev/video(\\d+)"};
  std::filesystem::path devices = "/dev";
  for (
    const std::filesystem::directory_entry& device :
    std::filesystem::directory_iterator(devices)
  ) {
    const auto& device_path_str = device.path().string();
    std::smatch m;
    if (!std::regex_match(device_path_str, m, video_pattern)) continue;

    int device_fd = open(device_path_str.c_str(), O_RDWR);
    if (device_fd < 0) throw std::runtime_error("Failed to open device.");

    v4l2_capability capability;
    if (ioctl(device_fd, VIDIOC_QUERYCAP, &capability) < 0) {
      throw std::runtime_error("Failed to query device capabilities.");
    }
    if ((capability.device_caps & V4L2_CAP_VIDEO_CAPTURE) == 0) continue;

    int camera_id = std::strtol(m[1].str().c_str(), nullptr, 10);
    camera_ids.push_back(camera_id);
  }

  camera_ids.shrink_to_fit();
  return camera_ids;
}

int main(int argc, char* argv[]) {
  const std::vector<int>& camera_ids = get_camera_ids();
  std::vector<std::unique_ptr<Camera>> cameras;
  cameras.reserve(camera_ids.size());
  for (const int id : camera_ids) {
    cameras.push_back(std::make_unique<Camera>(id));
  }

  auto start = std::chrono::steady_clock::now();
  auto frame_duration = std::chrono::nanoseconds(33333333);
  for (; frame_counter < 30 * 30; ++frame_counter) {
    auto next_shot = start + (frame_duration * (frame_counter + 1));
    for (std::unique_ptr<Camera>& camera : cameras) {
      camera->capture();
    }
    std::this_thread::sleep_until(next_shot);
  }
  auto elapsed = std::chrono::steady_clock::now() - start;
  double fps = (frame_counter * 1000.0) / (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count());
  std::cout << fps << "fps" << std::endl;
}
