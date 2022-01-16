#include <chrono>
#include <filesystem>
#include <iostream>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "cli/keys.h"
#include "episode/frames.h"
#include "episode/project.h"
#include "extraction/pose2d.h"

namespace {

using ::episode::CameraDirectory;
using ::episode::FrameRange;
using ::episode::Project;
using ::extraction::Pose2d;
using ::std::filesystem::path;

constexpr std::chrono::duration KEY_DELAY = std::chrono::milliseconds(33);

enum class CameraSocket {
  LEFT,
  RIGHT
};

struct State {
  Project project;
  std::vector<std::string> sessions;
  std::vector<CameraDirectory> cameras;
  std::size_t session_idx = 0;
  std::size_t camera_idx = 0;
  CameraSocket socket = CameraSocket::LEFT;
  FrameRange frames;
  std::int64_t frame_idx = 0;
  std::size_t frame_count;
  bool play = false;
  bool use_3d = false; // TODO(alaina) Implement 3d pose visualization.
};

struct Color {
  double r = 0.0;
  double g = 0.0;
  double b = 0.0;

  operator cv::Scalar() const {
    return {b, g, r};
  }
};

struct Font {
  int family = cv::FONT_HERSHEY_PLAIN;
  double scale = 1.0;
  Color color = {0.0, 0.0, 0.0};
  int thickness = 1;
  int line_mode = cv::LINE_AA;
};

void switch_socket(State& state) {
  if (state.socket == CameraSocket::LEFT) {
    state.socket = CameraSocket::RIGHT;
    state.frames =
      FrameRange{state.cameras.at(state.camera_idx).right_recording};
  } else {
    state.socket = CameraSocket::LEFT;
    state.frames =
      FrameRange{state.cameras.at(state.camera_idx).left_recording};
  }
  state.frame_count = state.frames.size();
  if (static_cast<std::size_t>(state.frame_idx) > state.frame_count) {
    state.frame_idx = state.frame_count - 1;
  }
}

void open_camera(State& state, std::size_t camera_idx) {
  camera_idx %= state.cameras.size();
  state.camera_idx = camera_idx;
  state.socket = CameraSocket::LEFT;
  state.frame_idx = 0;
  switch_socket(state);
}

void open_session(State& state, std::size_t session_idx) {
  session_idx %= state.sessions.size();
  state.session_idx = session_idx;
  state.cameras = state.project.cameras(state.sessions.at(session_idx));
  open_camera(state, 0);
}

void put_text(
  cv::Mat& image,
  cv::Point origin,
  std::string_view text,
  const Font& font = {}
) {
  cv::putText(
    image,
    std::string{text},
    origin,
    font.family,
    font.scale,
    font.color,
    font.thickness,
    font.line_mode
  );
}

void draw(cv::Mat& image, Color color, const Pose2d::Point& point) {
  double confidence_scale = 1.0 - point.confidence;
  color.r = 255 * confidence_scale;
  cv::Point center{static_cast<int>(point.x), static_cast<int>(point.y)};
  put_text(
    image,
    center,
    std::to_string(point.point_id),
    {.scale = 0.5, .color = color}
  );
}

void draw(
  cv::Mat& image,
  const Color& color,
  std::span<const Pose2d::Point> points
) {
  for (const Pose2d::Point& point : points) draw(image, color, point);
}

void draw(cv::Mat& image, const Pose2d& pose) {
  // TODO(alaina) Draw lines between points of the body and paw poses.
  draw(image, {0, 128, 0}, pose.body);
  draw(image, {0, 255, 0}, pose.face);
  draw(image, {0, 0, 128}, pose.right_paw);
  draw(image, {0, 0, 255}, pose.left_paw);
}

void draw(cv::Mat& image, std::span<const Pose2d> poses) {
  for (const Pose2d& pose : poses) draw(image, pose);
}

void draw(cv::Mat& image, const State& state) {
  put_text(image, {5, 15}, state.project.directory().string());

  std::string line =
    state.sessions.at(state.session_idx) + " : " +
    state.cameras.at(state.camera_idx).name + " : " +
    (state.socket == CameraSocket::LEFT ? "left" : "right");
  put_text(image, {5, 30}, line);

  line =
    std::to_string(state.frame_idx) + " : " + std::to_string(state.frame_count);
  put_text(image, {5, 45}, line);
}

void display(const State& state) {
  path img_path = state.frames[state.frame_idx];
  cv::Mat image = cv::imread(img_path.string());

  path pose_path = state.project.pose_path_for_frame(img_path);
  if (std::filesystem::exists(pose_path)) {
    draw(image, extraction::read_poses2d(pose_path));
  }

  draw(image, state);
  cv::imshow("Visualizer", image);
}

}

int main(int argc, const char** argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " [PROJECT_PATH]" << std::endl;
    return -1;
  }

  State state = {.project = Project::open(argv[1]), .frames = FrameRange{""}};
  state.sessions = state.project.sessions();
  open_session(state, 0);
  display(state);

  cli::Key key;
  do {
    key = cli::wait_key(KEY_DELAY);
    bool update_display = true;
    if (key == cli::Key::ONE) ++state.frame_idx;
    else if (key == cli::Key::TWO) state.frame_idx += 5;
    else if (key == cli::Key::THREE) state.frame_idx += 10;
    else if (key == cli::Key::FOUR) state.frame_idx += 100;
    else if (key == cli::Key::Q) --state.frame_idx;
    else if (key == cli::Key::W) state.frame_idx -= 5;
    else if (key == cli::Key::E) state.frame_idx -= 10;
    else if (key == cli::Key::R) state.frame_idx -= 100;
    else if (key == cli::Key::A) open_session(state, state.session_idx + 1);
    else if (key == cli::Key::S) open_camera(state, state.camera_idx + 1);
    else if (key == cli::Key::D) switch_socket(state);
    else if (key == cli::Key::M) state.use_3d = !state.use_3d;
    else if (key == cli::Key::SPACE) state.play = !state.play;
    else update_display = false;

    if (state.play) {
      ++state.frame_idx;
      update_display = true;
    }
    while (state.frame_idx < 0) state.frame_idx += state.frame_count;
    state.frame_idx %= state.frame_count;

    if (update_display) display(state);
  } while (key != cli::Key::ESC);

  return 0;
}
