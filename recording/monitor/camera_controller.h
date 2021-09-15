#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <opencv2/core.hpp>
#include <string_view>
#include <thread>

#include "recording/monitor/camera_stream.h"
#include "recording/monitor/frame_saver.h"

namespace recording {

class CameraController {
public:
  // Creates a new camera controller that will save frames from the given host
  // to the given directory.
  //
  // @param camera_host
  //  The hostname of the remote camera.
  // @param base_directory
  //  The path to the root directory for this camera.
  //
  // @return
  //  A new camera controller that is actively downloading frames from the
  //  remote camera.
  static std::unique_ptr<CameraController> create(
    std::string_view camera_host,
    const std::filesystem::path& base_directory
  );

  ~CameraController();

  CameraController(CameraController&& other) = delete;
  CameraController& operator=(CameraController&& other) = delete;

  CameraController(const CameraController&) = delete;
  CameraController& operator=(const CameraController&) = delete;

  // Returns true if downloaded frames are being persisted on disk.
  bool recording() const { return _record; }
  // Begins persisting frames to disk.
  void start_recording() { _record = true; }
  // Stops peristing frames to disk.
  void stop_recording() { _record = false; }

  // Returns true if frames are being copied into memory for display purposes.
  bool displaying() const { return _display; }
  // Begins copying frames into memory.
  void display() { _display = true; }
  // Stops copying frames into memory.
  void hide() { _display = false; }
  // Returns the latest frame copied into memory.
  cv::Mat frame() const;

private:
  explicit CameraController(
    std::string_view camera_host,
    const std::filesystem::path& output_dir
  ):
    _stream{CameraStream::connect(camera_host)},
    _saver{_stream, output_dir},
    _stream_thread{[this]() { _camera_thread_main(); }}
  {}

  void _camera_thread_main();

  void _read_frame_data(const std::filesystem::path& frame_path);

  CameraStream _stream;
  FrameSaver _saver;
  std::thread _stream_thread;
  std::atomic_bool _continue = true;
  std::atomic_bool _record = false;
  std::atomic_bool _display = false;
  mutable std::mutex _frame_guard;
  cv::Mat _frame;
};

}
