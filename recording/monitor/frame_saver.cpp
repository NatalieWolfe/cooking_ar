#include "recording/monitor/frame_saver.h"

#include <cstdio>
#include <fstream>
#include <string>

namespace recording {

std::uint64_t FrameSaver::save_frame() {
  char frame_name[12] = {0};
  std::snprintf(
    frame_name, sizeof(frame_name),
    "%06lu.jpg",
    ++_frame_id
  );
  std::ofstream output{_output_dir / frame_name, std::ios::binary};
  _stream.read_frame(output);
  output.flush();

  return _frame_id;
}

}
