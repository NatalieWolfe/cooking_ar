#include "recording/monitor/frame_saver.h"

#include <cstdio>
#include <fstream>
#include <string>

namespace recording {

std::uint64_t FrameSaver::save_frame() {
  std::ofstream output{frame_path(++_frame_id), std::ios::binary};
  _stream.read_frame(output);
  output.flush();

  return _frame_id;
}

std::filesystem::path FrameSaver::frame_path(std::uint64_t frame_id) const {
  char frame_name[16] = {0};
  std::snprintf(frame_name, sizeof(frame_name), "%06lu.jpg", frame_id);
  return _output_dir / frame_name;
}

}
