#pragma once

#include <cstdint>
#include <filesystem>

#include "recording/monitor/camera_stream.h"

namespace recording {

class FrameSaver {
public:
  explicit FrameSaver(CameraStream& stream, std::filesystem::path output_dir):
    _stream{stream},
    _output_dir{std::move(output_dir)}
  {}

  FrameSaver(const FrameSaver&) = delete;
  FrameSaver& operator=(const FrameSaver&) = delete;

  FrameSaver(FrameSaver&& other) = default;
  FrameSaver& operator=(FrameSaver&& other) = default;

  // Writes a frame to a file in the configured output directory.
  //
  // @return
  //  The number of the saved frame. Each call will increment by one.
  std::uint64_t save_frame();

  // Creates the frame path for the given frame.
  //
  // @param frame_id
  //  The ID of the frame whose path will be returned.
  //
  // @return
  //  The path to the file containing the frame's image.
  std::filesystem::path frame_path(std::uint64_t frame_id) const;

private:
  CameraStream& _stream;
  std::filesystem::path _output_dir;
  std::uint64_t _frame_id = 0;
};

}
