#include "src/keys.h"

#include <chrono>
#include <opencv2/highgui.hpp>

Key wait_key(std::chrono::milliseconds duration) {
  return static_cast<Key>(cv::waitKey(duration.count()));
}
