#include <chrono>
#include <filesystem>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "cli/keys.h"
#include "episode/cameras.h"
#include "episode/frames.h"
#include "episode/project.h"
#include "extraction/pose2d.h"
#include "extraction/pose3d.h"
#include "lw/base/base.h"
#include "lw/flags/flags.h"

LW_FLAG(
  double, min_confidence, 0.5,
  "Minimum confidence per-point required for display."
);

namespace {

using ::episode::CameraCalibration;
using ::episode::CameraDirectory;
using ::episode::FrameRange;
using ::episode::Project;
using ::extraction::Pose2d;
using ::extraction::Pose3d;
using ::std::filesystem::exists;
using ::std::filesystem::path;

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
  FrameRange left_frames;
  FrameRange right_frames;
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

constexpr std::chrono::duration KEY_DELAY = std::chrono::milliseconds(33);
constexpr Color BODY{0, 128, 0};
constexpr Color FACE{0, 255, 0};
constexpr Color RIGHT_PAW{0, 0, 128};
constexpr Color LEFT_PAW{0, 0, 255};

void open_frames(State& state) {
  state.left_frames =
      FrameRange{state.cameras.at(state.camera_idx).left_recording};
  state.right_frames =
      FrameRange{state.cameras.at(state.camera_idx).right_recording};
  state.frame_count = state.left_frames.size();
  if (static_cast<std::size_t>(state.frame_idx) > state.frame_count) {
    state.frame_idx = state.frame_count - 1;
  }
}

void open_camera(State& state, std::size_t camera_idx) {
  camera_idx %= state.cameras.size();
  state.camera_idx = camera_idx;
  state.frame_idx = 0;
  open_frames(state);
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

// -------------------------------------------------------------------------- //
// Pose2d

void draw(cv::Mat& image, Color color, const Pose2d::Point& point) {
  if (point.confidence < lw::flags::min_confidence) return;

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
  draw(image, BODY, pose.body);
  draw(image, FACE, pose.face);
  draw(image, RIGHT_PAW, pose.right_paw);
  draw(image, LEFT_PAW, pose.left_paw);
}

void draw(cv::Mat& image, std::span<const Pose2d> poses) {
  for (const Pose2d& pose : poses) draw(image, pose);
}

// -------------------------------------------------------------------------- //
// Pose3d

void draw(
  cv::Mat& image,
  const cv::Mat& rvec,
  const CameraCalibration::Parameters& params,
  const Color& color,
  std::span<const Pose3d::Point> points
) {
  std::vector<cv::Point3d> points_3d;
  std::vector<cv::Point2d> points_2d;
  points_3d.reserve(points.size());
  points_2d.reserve(points.size());
  for (const Pose3d::Point& p : points) points_3d.emplace_back(p.x, p.y, p.z);
  cv::projectPoints(
    points_3d,
    rvec,
    params.translation,
    params.matrix,
    params.distortion,
    points_2d
  );

  int point_idx = 0;
  for (const cv::Point2d& p : points_2d) {
    const Pose3d::Point& pose_point = points[point_idx++];
    draw(
      image,
      color,
      Pose2d::Point{
        .point_id = pose_point.point_id,
        .x = p.x,
        .y = p.y,
        .confidence = pose_point.confidence
      }
    );
  }
}

void draw(
  cv::Mat& image,
  const cv::Mat& rvec,
  const CameraCalibration::Parameters& params,
  const Pose3d& pose
) {
  // TODO(alaina) Draw lines between points of the body and paw poses.
  draw(image, rvec, params, BODY, pose.body);
  draw(image, rvec, params, FACE, pose.face);
  draw(image, rvec, params, RIGHT_PAW, pose.right_paw);
  draw(image, rvec, params, LEFT_PAW, pose.left_paw);
}

void draw(
  cv::Mat& image,
  const CameraCalibration::Parameters& params,
  std::span<const Pose3d> poses
) {
  cv::Mat rvec;
  cv::Rodrigues(params.rotation, rvec);
  for (const Pose3d& pose : poses) draw(image, rvec, params, pose);
}

// -------------------------------------------------------------------------- //
// State

void draw(cv::Mat& image, const State& state) {
  put_text(image, {5, 15}, state.project.directory().string());

  std::string line =
    state.sessions.at(state.session_idx) + " : " +
    state.cameras.at(state.camera_idx).name;
  put_text(image, {5, 30}, line);

  line =
    std::to_string(state.frame_idx) + " : " +
    std::to_string(state.frame_count) + " : " +
    (state.use_3d ? "3d" : "2d");
  put_text(image, {5, 45}, line);
}

void render_3d(
  cv::Mat& image,
  const State& state,
  const path& img_path,
  CameraSocket socket
) {
  path pose_path = state.project.pose3d_path_for_frame(img_path);
  if (exists(pose_path)) {
    CameraCalibration calibration = episode::load_camera_calibration(
      state.cameras.at(state.camera_idx).calibration_file
    );
    const CameraCalibration::Parameters& params =
      socket == CameraSocket::LEFT ?
      calibration.left : calibration.right;
    draw(image, params, extraction::read_poses3d(pose_path));
  }
}

void render_2d(cv::Mat& image, const State& state, const path& img_path) {
  path pose_path = state.project.pose_path_for_frame(img_path);
  if (exists(pose_path)) {
    draw(image, extraction::read_poses2d(pose_path));
  }
}

void display(const State& state) {
  path left_img_path = state.left_frames[state.frame_idx];
  cv::Mat left_image = cv::imread(left_img_path.string());
  path right_img_path = state.right_frames[state.frame_idx];
  cv::Mat right_image = cv::imread(right_img_path.string());

  if (state.use_3d) {
    render_3d(left_image, state, left_img_path, CameraSocket::LEFT);
    render_3d(right_image, state, right_img_path, CameraSocket::RIGHT);
  } else {
    render_2d(left_image, state, left_img_path);
    render_2d(right_image, state, right_img_path);
  }

  cv::Mat image;
  cv::hconcat(left_image, right_image, image);
  draw(image, state);
  cv::imshow("Visualizer", image);
}

}

int main(int argc, const char** argv) {
  if (!lw::init(&argc, argv)) return -2;
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " [PROJECT_PATH]" << std::endl;
    return -1;
  }

  State state = {
    .project = Project::open(argv[1]),
    .left_frames = FrameRange{""},
    .right_frames = FrameRange{""}
  };
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
