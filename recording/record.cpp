#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <thread>

#include <depthai/depthai.hpp>

#include "episode/project.h"
#include "lf/queue.h"
#include "recording/oakd_camera.h"

namespace {

using ::episode::CameraDirectory;
using ::episode::Project;
using ::recording::OakDCamera;
using ::recording::OakDFrames;
using ::std::chrono::duration;
using ::std::chrono::duration_cast;
using ::std::chrono::milliseconds;
using ::std::chrono::seconds;
using ::std::chrono::steady_clock;
using ::std::chrono::time_point;

constexpr std::size_t FRAME_BUFFER = 1000;

std::string frame_file(std::size_t id) {
  std::stringstream name;
  name << std::setw(8) << std::setfill('0') << std::right << id << ".png";
  return name.str();
}

}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " [PROJECT_PATH]" << std::endl;
    return -1;
  }

  Project project = Project::open(argv[1]);
  std::atomic_bool run = true;
  std::size_t counter = 0;
  lf::Queue<OakDFrames> frames{FRAME_BUFFER};

  std::thread frame_saver{[&]() {
    const CameraDirectory& cam = project.add_camera("oakd-lite");
    std::string last_message;
    while (run || !frames.empty()) {
      std::optional<OakDFrames> frame = frames.pop();
      if (!frame) continue;

      cv::Mat right = frame->right->getCvFrame();
      cv::Mat left = frame->left->getCvFrame();

      std::string filename = frame_file(++counter);
      cv::imwrite(cam.right_recording / filename, right);
      cv::imwrite(cam.left_recording / filename, left);

      cv::imshow("right", right);
      cv::imshow("left", left);

      int key = cv::waitKey(1);
      if (key == 'q' || key == 'Q') run = false;
      for (std::size_t i = 0; i < last_message.size(); ++i) {
        std::cout << '\b';
      }
      std::stringstream out_buffer;
      out_buffer << frames.size() << ":" << counter;
      last_message = out_buffer.str();
      std::cout << last_message << std::flush;
    }
  }};

  OakDCamera cam = OakDCamera::make();
  time_point start = steady_clock::now();
  while (run) {
    frames.push(cam.get());
  }
  time_point end = steady_clock::now();

  std::cout << std::endl << "Exiting..." << std::endl;
  frame_saver.join();
  std::cout << frames.size() << " frames dropped." << std::endl;

  duration elapsed = end - start;
  double fps =
    static_cast<double>(counter) /
    static_cast<double>(duration_cast<seconds>(elapsed).count());
  std::cout
    << counter << " frames over "
    << duration_cast<milliseconds>(elapsed).count() << "ms @ " << fps << " fps"
    << std::endl;

  return 0;
}
