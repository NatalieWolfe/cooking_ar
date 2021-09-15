#include <chrono>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "recording/monitor/camera_controller.h"
#include "recording/monitor/charuco.h"
#include "recording/monitor/input.h"

namespace {
using namespace std::chrono_literals;
using namespace recording;

struct Camera {
  std::unique_ptr<CameraController> controller;
  std::unique_ptr<CharucoCalibrator> calibrator;
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
  run_camera_calibration(cameras);
}
