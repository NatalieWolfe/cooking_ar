#include <atomic>
#include <condition_variable>
#include <exception>
#include <filesystem>
#include <functional>
#include <iostream>
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/videoio.hpp>
#include <thread>
#include <vector>

#include "src/cameras.h"
#include "src/files.h"
#include "src/timing.h"

static std::atomic_size_t frame_counter = 0;

class Camera {
private:
  enum class Action {
    NONE,
    EXIT,
    CAPTURE
  };

public:
  explicit Camera(int camera_id):
    _camera_id{camera_id},
    _save_path{get_recordings_path(camera_id)}
  {
    _camera_input.open(_camera_id);
    _camera_input.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    _camera_input.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    _thread = std::thread{std::bind(&Camera::_thread_loop, this)};
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

int main(int argc, char* argv[]) {
  const std::vector<CameraDevice>& devices = get_camera_devices();
  std::vector<std::unique_ptr<Camera>> cameras;
  cameras.reserve(devices.size());
  for (const CameraDevice& device : devices) {
    std::cout << device.device_path.string() << ": " << device.name << std::endl;
    cameras.push_back(std::make_unique<Camera>(device.camera_id));
  }

  // Make an initial capture and pause to wake up all the cameras.
  for (std::unique_ptr<Camera>& camera : cameras) {
    camera->capture();
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));

  auto start = steady_clock::now();
  auto frame_duration = std::chrono::nanoseconds(33333333); // 30 fps
  for (; frame_counter < 1000; ++frame_counter) {
    auto next_shot = start + (frame_duration * (frame_counter + 1));
    for (std::unique_ptr<Camera>& camera : cameras) {
      camera->capture();
    }
    std::this_thread::sleep_until(next_shot);
  }
  auto elapsed = steady_clock::now() - start;
  std::cout << to_fps(frame_counter, elapsed) << "fps" << std::endl;
}
