#pragma once

#include <cstdint>
#include <filesystem>

#include "recording/monitor/camera_stream.h"

namespace recording {

class FrameSaver {
public:
  explicit FrameSaver(CameraStream stream, std::filesystem::path output_dir):
    _stream{std::move(stream)},
    _output_dir{std::move(output_dir)}
  {}

  FrameSaver(const FrameSaver&) = delete;
  FrameSaver& operator=(const FrameSaver&) = delete;

  FrameSaver(FrameSaver&& other) = default;
  FrameSaver& operator=(FrameSaver&& other) = default;

  // Writes a frame to a file in the configured output directory.
  //
  // @return The number of the saved frame. Each call will increment by one.
  std::uint64_t save_frame();

private:
  CameraStream _stream;
  std::filesystem::path _output_dir;
  std::uint64_t _frame_id = 0;
};

}
