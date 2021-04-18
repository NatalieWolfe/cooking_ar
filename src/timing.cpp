#include "src/timing.h"

#include <chrono>

using std::chrono::duration_cast;
using std::chrono::milliseconds;

double to_fps(std::size_t frame_count, const steady_clock::duration& duration) {
  return
    (frame_count * 1000.0) / (duration_cast<milliseconds>(duration).count());
}
