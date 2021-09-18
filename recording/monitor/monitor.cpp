#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "recording/monitor/camera_controller.h"
#include "recording/monitor/charuco.h"
#include "recording/monitor/input.h"

namespace {
using namespace std::chrono_literals;
using namespace recording;

constexpr std::size_t MAX_CALIBRATION_SET = 25;
constexpr double ERROR_RATE_GOAL = 0.1;
constexpr double FILTER_ALPHA = 0.1;

struct Camera {
  std::unique_ptr<CameraController> controller;
  std::unique_ptr<CharucoCalibrator> calibrator;
};

struct Frame {
  cv::Mat image;
  cv::Mat corners;
  cv::Mat corner_ids;
};

void put_text(cv::Mat& image, cv::Point origin, std::string_view text) {
  std::stringstream stream{std::string{text}};
  std::string line;
  for (; stream >> line; origin.y += 10) {
    cv::putText(
      image,
      line,
      origin,
      cv::FONT_HERSHEY_PLAIN,
      /*fontScale=*/0.9,
      /*color=*/{0, 0, 0},
      /*thickness=*/2,
      cv::LINE_AA
    );
    cv::putText(
      image,
      line,
      origin,
      cv::FONT_HERSHEY_PLAIN,
      /*fontScale=*/0.9,
      /*color=*/{255, 255, 255},
      /*thickness=*/1,
      cv::LINE_AA
    );
  }
}

void draw_corners(
  cv::InputOutputArray image,
  cv::Scalar color,
  const cv::Mat& corners
) {
  for (int i = 0; i < corners.size[0]; ++i) {
    cv::circle(
      image,
      cv::Point{
        static_cast<int>(corners.at<float>(i, 0)),
        static_cast<int>(corners.at<float>(i, 1))
      },
      /*radius=*/4,
      color,
      /*thickness=*/1,
      cv::LINE_AA
    );
  }
}

double estimate_grid_quality(
  const std::vector<Frame>& frames,
  std::size_t exclude = -1
) {
  int grid_size = 10;
  int x_grid_step = frames.front().image.cols / grid_size;
  int y_grid_step = frames.front().image.rows / grid_size;
  std::vector<int> points_in_cell(grid_size * grid_size, 0);

  for (std::size_t i = 0; i < MAX_CALIBRATION_SET; ++i) {
    if (i == exclude) continue;
    const cv::Mat& corners = frames.at(i).corners;
    for (int j = 0; j < corners.size[0]; ++j) {
      int x = static_cast<int>(corners.at<float>(j, 0) / x_grid_step);
      int y = static_cast<int>(corners.at<float>(j, 1) / y_grid_step);
      ++points_in_cell[(x * grid_size) + y];
    }
  }

  cv::Mat mean;
  cv::Mat std_dev;
  cv::meanStdDev(points_in_cell, mean, std_dev);

  return mean.at<double>(0) / (std_dev.at<double>(0) + 1e-7);
}

std::pair<double, std::size_t> pick_worst(const std::vector<Frame>& frames) {
  CharucoCalibrator calibrator{get_charuco_board()};
  for (std::size_t i = 0; i < MAX_CALIBRATION_SET; ++i) {
    calibrator.add_corners(frames.at(i).corners, frames.at(i).corner_ids);
  }
  calibrator.set_latest_frame(frames.front().image, /*extract_board=*/false);
  std::vector<double> frame_errors(frames.size(), 0.0);
  double error_rate = calibrator.calibrate(frame_errors);

  double grid_quality = estimate_grid_quality(frames);

  std::size_t worst_frame_idx = -1;
  double worst_value = -420.69;
  for (std::size_t i = 0; i < MAX_CALIBRATION_SET; ++i) {
    double grid_quality_delta =
      estimate_grid_quality(frames, /*exclude=*/i) - grid_quality;
    double frame_error = frame_errors.at(i);
    double frame_value =
      (frame_error * FILTER_ALPHA) +
      (grid_quality_delta * (1.0 - FILTER_ALPHA));

    if (frame_value > worst_value) {
      worst_value = frame_value;
      worst_frame_idx = i;
    }
  }
  return std::make_pair(error_rate, worst_frame_idx);
}

void calibrate(Camera& camera) {
  CharucoCalibrator detector{get_charuco_board()};
  camera.controller->display();
  std::mutex frame_guard;
  std::vector<Frame> frames;
  std::atomic_size_t frame_count = 0;
  std::atomic_bool thread_continue = true;
  std::atomic<double> previous_error_rate = 420.69;

  frames.reserve(MAX_CALIBRATION_SET * 100);

  std::thread calibrator_thread{[&]() {
    while (thread_continue) {
      if (frame_count <= MAX_CALIBRATION_SET) continue;

      double error_rate;
      std::size_t worst_idx;
      std::tie(error_rate, worst_idx) = pick_worst(frames);

      std::scoped_lock<std::mutex> lock{frame_guard};
      previous_error_rate = error_rate;
      frames.erase(frames.begin() + worst_idx);
      --frame_count;
    }
  }};

  Key key;
  std::size_t counter = 0;
  while (
    (key = wait_key(50ms)) != Key::ESC &&
    previous_error_rate > ERROR_RATE_GOAL
  ) {
    cv::Scalar color{255, 0, 255};
    Frame frame;
    frame.image = camera.controller->frame();
    if (frame.image.empty()) continue;
    ++counter;

    cv::Mat display = frame.image.clone();
    int grid_size = display.size().height / 8;
    for (int x = 0; x < display.size().width; x += grid_size) {
      cv::line(display, {x, 0}, {x, display.size().height}, color);
    }
    for (int y = 0; y < display.size().height; y += grid_size) {
      cv::line(display, {0, y}, {display.size().width, y}, color);
    }
    put_text(display, {10, 10}, "err: " + std::to_string(previous_error_rate));
    cv::imshow("Calibrator", display);

    if (counter % 10) continue;
    auto charuco = detector.extract_charuco(frame.image);
    std::scoped_lock<std::mutex> lock{frame_guard};
    if (charuco && charuco->first.rows > 6) {
      std::tie(frame.corners, frame.corner_ids) = std::move(*charuco);
      frames.push_back(frame);
      ++frame_count;
    }

    cv::Mat visualizer = cv::Mat::zeros(display.size(), display.type());
    for (std::size_t i = 0; i < frames.size(); ++i) {
      color =
        i < MAX_CALIBRATION_SET ? cv::Scalar{0, 255, 0} : cv::Scalar{255, 0, 0};
      draw_corners(visualizer, color, frames.at(i).corners);
    }
    cv::imshow("Visualization", visualizer);
  }

  thread_continue = false;
  calibrator_thread.join();
}

void run_auto_calibration(std::span<Camera> cameras) {
  for (Camera& camera : cameras) {
    calibrate(camera);
  }
  cv::destroyAllWindows();
}

void run_camera_calibration(std::span<Camera> cameras) {
  Key key;
  bool render_undistorted = false;
  bool flip = false;
  std::size_t camera_idx = 0;

  cameras[camera_idx].controller->display();

  while ((key = wait_key(50ms)) != Key::ESC) {
    Camera* camera = &cameras[camera_idx];

    switch (key) {
      case Key::RIGHT: {
        camera_idx += 2;
        [[fallthrough]];
      }
      case Key::LEFT: {
        --camera_idx;
        camera_idx %= cameras.size();

        camera->controller->hide();
        camera = &cameras[camera_idx];
        camera->controller->display();
        if (render_undistorted && !camera->calibrator->calibrated()) {
          render_undistorted = false;
        }
        break;
      }

      case Key::SPACE: {
        camera->calibrator->save_latest_frame();
        break;
      }

      case Key::X: {
        camera->calibrator->drop_latest_frame();
        break;
      }

      case Key::Z: {
        render_undistorted =
          camera->calibrator->calibrated() && !render_undistorted;
        break;
      }

      case Key::F: {
        flip = !flip;
        break;
      }

      case Key::NONE: break;

      default: {
        // std::cout << "Unknown key: " << static_cast<int>(key) << std::endl;
        break;
      }
    }

    cv::Mat frame = camera->controller->frame();
    camera->calibrator->set_latest_frame(frame);
    if (render_undistorted) {
      frame = camera->calibrator->undistort_latest_frame();
    }

    cv::Scalar color{255, 0, 255};
    if (camera->calibrator->calibrated()) color = {0, 255, 0};

    cv::Mat display = frame.clone();
    if (flip) cv::flip(display, display, 1);

    int grid_size = display.size().height / 8;
    for (int x = 0; x < display.size().width; x += grid_size) {
      cv::line(display, {x, 0}, {x, display.size().height}, color);
    }
    for (int y = 0; y < display.size().height; y += grid_size) {
      cv::line(display, {0, y}, {display.size().width, y}, color);
    }
    put_text(display, {10, 10}, camera->calibrator->debug_text());
    cv::imshow("Calibrator", display);

    cv::Mat visualizer = cv::Mat::zeros(display.size(), display.type());
    camera->calibrator->draw_saved_corners(visualizer, color);
    camera->calibrator->draw_latest_corners(visualizer, {255, 0, 64});
    if (flip) cv::flip(visualizer, visualizer, 1);
    cv::imshow("Visualization", visualizer);
  }
  cv::destroyAllWindows();
}

}

int main() {
  auto calibration_board = get_charuco_board();

  std::vector<Camera> cameras;
  cameras.push_back({
    .controller = CameraController::create("camera1", "/tmp"),
    .calibrator = std::make_unique<CharucoCalibrator>(calibration_board)
  });
  run_auto_calibration(cameras);
}
