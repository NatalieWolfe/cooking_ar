#include "src/timing.h"

#include <chrono>
#include <sstream>
#include <string>

using std::chrono::duration_cast;
using std::chrono::hours;
using std::chrono::minutes;
using std::chrono::seconds;
using std::chrono::milliseconds;

double to_fps(std::size_t frame_count, const steady_clock::duration& duration) {
  return
    (frame_count * 1000.0) / (duration_cast<milliseconds>(duration).count());
}

std::string to_hms(steady_clock::duration duration) {
  std::stringstream stream;

  if (duration >= hours(1)) {
    hours chunk = duration_cast<hours>(duration);
    stream << chunk.count() << 'h';
    duration -= chunk;
  }
  if (duration >= minutes(1)) {
    minutes chunk = duration_cast<minutes>(duration);
    stream << chunk.count() << 'm';
    duration -= chunk;
  }
  seconds chunk = duration_cast<seconds>(duration);
  duration -= chunk;
  double secs =
    chunk.count() + (duration_cast<milliseconds>(duration).count() / 1000.0);
  stream << secs << 's';

  return stream.str();
}
