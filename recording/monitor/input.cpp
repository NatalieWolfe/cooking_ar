#include "recording/monitor/input.h"

#include <chrono>
#include <opencv2/highgui.hpp>

namespace recording {

Key wait_key(std::chrono::milliseconds duration) {
  return static_cast<Key>(cv::waitKey(duration.count()));
}

}
