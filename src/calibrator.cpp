#include <chrono>
#include <iostream>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>
#include <vector>

#include "src/cameras.h"
#include "src/files.h"
#include "src/keys.h"

using namespace std::chrono_literals;

constexpr std::size_t MIN_CALIBRATION_FRAMES = 5;

cv::Ptr<cv::aruco::CharucoBoard> get_board() {
  static cv::Ptr<cv::aruco::CharucoBoard> board =
    cv::aruco::CharucoBoard::create(
      5, 7,
      0.03f, 0.015f,
      cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250)
    );
  return board;
}

cv::VideoCapture open_camera(int camera_id) {
  cv::VideoCapture camera;
  camera.open(camera_id);
  camera.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
  camera.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);
  return camera;
}

class CharucoCalibrator {
public:
  explicit CharucoCalibrator(CameraDevice device):
    _parameters{.device = std::move(device)},
    _camera{open_camera(_parameters.device.camera_id)}
  {}

  const CameraParameters& parameters() const { return _parameters; }
  const CameraDevice& device() const { return _parameters.device; }
  const cv::Mat& frame() const { return _display_frame; }
  const cv::Mat& last_corners() const { return _last_charuco_corners; }
  const std::vector<cv::Mat>& saved_corners() const {
    return _saved_charuco_corners;
  }
  bool calibrated() const {
    return !_parameters.matrix.empty() && !_parameters.distortion.empty();
  }

  void save_frame() {
    if (_last_charuco_corners.empty()) return;

    _last_charuco_corners.copyTo(_saved_charuco_corners.emplace_back());
    _last_charuco_ids.copyTo(_saved_charuco_ids.emplace_back());
    std::cout << "Saving frame state " << _saved_charuco_ids.size();

    if (_saved_charuco_corners.size() >= MIN_CALIBRATION_FRAMES) {
      double avg_error = _calibrate();
      std::cout << "; calibrated with " << avg_error << " average error";
    }
    std::cout << '.' << std::endl;
  }

  void drop_frame() {
    if (_saved_charuco_corners.empty()) return;

    std::cout << "Dropping last frame state";
    _saved_charuco_corners.pop_back();
    _saved_charuco_ids.pop_back();

    if (_saved_charuco_corners.size() >= MIN_CALIBRATION_FRAMES) {
      double avg_error = _calibrate();
      std::cout << "; calibrated with " << avg_error << " average error";
    }
    std::cout << '.' << std::endl;
  }

  void undistort_frame() {
    _last_charuco_ids = cv::Mat{};
    _last_charuco_corners = cv::Mat{};

    _camera >> _frame;

    cv::Mat undistorted_map_1;
    cv::Mat undistorted_map_2;
    cv::Mat optimal_matrix = cv::getOptimalNewCameraMatrix(
      _parameters.matrix,
      _parameters.distortion,
      _frame.size(),
      0.0,
      _frame.size()
    );
    cv::initUndistortRectifyMap(
      _parameters.matrix,
      _parameters.distortion,
      cv::noArray(),
      optimal_matrix,
      _frame.size(),
      CV_16SC2,
      undistorted_map_1,
      undistorted_map_2
    );

    cv::remap(
      _frame,
      _display_frame,
      undistorted_map_1,
      undistorted_map_2,
      cv::INTER_CUBIC
    );
  }

  void detect_board() {
    _display_frame = _detect_charuco();
  }

  void grab_frame() {
    _camera >> _frame;
  }

private:
  double _calibrate() {
    return cv::aruco::calibrateCameraCharuco(
      _saved_charuco_corners, _saved_charuco_ids, get_board(), _frame.size(),
      _parameters.matrix, _parameters.distortion,
      _parameters.rotation, _parameters.translation,
      cv::noArray(),  // std deviation intrinsics
      cv::noArray(),  // std deviation extrinsics
      cv::noArray()   // per view errors
    );
  }

  cv::Mat _detect_charuco() {
    cv::Mat display_image;
    _frame.copyTo(display_image);
    auto board = get_board();
    cv::Ptr<cv::aruco::DetectorParameters> params =
      cv::aruco::DetectorParameters::create();
    params->cornerRefinementMethod = cv::aruco::CORNER_REFINE_CONTOUR;

    std::vector<int> marker_ids;
    std::vector<std::vector<cv::Point2f>> marker_corners;
    cv::aruco::detectMarkers(
      _frame,
      board->dictionary,
      marker_corners,
      marker_ids,
      params
    );

    if (marker_ids.empty()) return display_image;
    cv::aruco::drawDetectedMarkers(display_image, marker_corners, marker_ids);
    cv::aruco::interpolateCornersCharuco(
      marker_corners,
      marker_ids,
      _frame,
      board,
      _last_charuco_corners,
      _last_charuco_ids
    );

    if (_last_charuco_ids.empty()) return display_image;
    cv::aruco::drawDetectedCornersCharuco(
      display_image,
      _last_charuco_corners,
      _last_charuco_ids,
      cv::Scalar{255, 0, 0}
    );

    if (calibrated() && _estimate_pose()) {
      cv::aruco::drawAxis(
        display_image,
        _parameters.matrix,
        _parameters.distortion,
        _parameters.rotation,
        _parameters.translation,
        board->getSquareLength()
      );
    }

    return display_image;
  }

  bool _estimate_pose() {
    return cv::aruco::estimatePoseCharucoBoard(
      _last_charuco_corners,
      _last_charuco_ids,
      get_board(),
      _parameters.matrix,
      _parameters.distortion,
      _parameters.rotation,
      _parameters.translation
    );
  }

  CameraParameters _parameters;
  cv::VideoCapture _camera;
  cv::Mat _frame;
  cv::Mat _display_frame;
  cv::Mat _last_charuco_ids;
  cv::Mat _last_charuco_corners;
  std::vector<cv::Mat> _saved_charuco_ids;
  std::vector<cv::Mat> _saved_charuco_corners;
};

std::vector<CharucoCalibrator> get_calibrators() {
  std::vector<CameraDevice> devices = get_camera_devices();
  std::vector<CharucoCalibrator> calibrators;
  for (CameraDevice& device : devices) {
    calibrators.emplace_back(std::move(device));
  }
  return calibrators;
}

void draw_corners(
  cv::Mat& image,
  const cv::Scalar& color,
  const cv::Mat& corners
) {
  for (int i = 0; i < corners.size[0]; ++i) {
    cv::circle(
      image,
      cv::Point{
        static_cast<int>(corners.at<float>(i, 0)),
        static_cast<int>(corners.at<float>(i, 1))
      },
      4, // Point size,
      color,
      1,
      cv::LINE_AA
    );
  }
}

void run_camera_calibration(std::vector<CharucoCalibrator>& calibrators) {
  Key key;
  bool render_undistorted = false;
  std::size_t camera_idx = 0;

  while (key = wait_key(50ms), key != Key::ESC) {
    CharucoCalibrator* calibrator = &calibrators[camera_idx];

    switch (key) {
      case Key::RIGHT: {
        camera_idx += 2;
        [[fallthrough]];
      }
      case Key::LEFT: {
        --camera_idx;
        camera_idx %= calibrators.size();
        calibrator = &calibrators[camera_idx];
        if (render_undistorted && !calibrator->calibrated()) {
          render_undistorted = false;
        }
        std::cout
          << "Switched to camera " << calibrator->device().camera_id
          << std::endl;
        break;
      }

      case Key::SPACE: {
        calibrator->save_frame();
        break;
      }

      case Key::X: {
        calibrator->drop_frame();
        break;
      }

      case Key::Z: {
        render_undistorted = calibrator->calibrated() && !render_undistorted;
        std::cout
          << "Rendering undistorted: " << std::boolalpha << render_undistorted
          << std::endl;
        break;
      }

      case Key::NONE: break;

      default: {
        std::cout << "Unknown key: " << static_cast<int>(key) << std::endl;
        break;
      }
    }

    calibrator->grab_frame();
    if (render_undistorted) {
      calibrator->undistort_frame();
    } else {
      calibrator->detect_board();
    }

    const cv::Scalar color{255, 0, 255};
    cv::Mat mirror;
    cv::flip(calibrator->frame(), mirror, 1);
    int grid_size = mirror.size().height / 8;
    for (int x = 0; x < mirror.size().width; x += grid_size) {
      cv::line(mirror, {x, 0}, {x, mirror.size().height}, color);
    }
    for (int y = 0; y < mirror.size().height; y += grid_size) {
      cv::line(mirror, {0, y}, {mirror.size().width, y}, color);
    }
    cv::imshow("Calibrator", mirror);

    cv::Mat visualizer = cv::Mat::zeros(mirror.size(), mirror.type());
    for (const cv::Mat& corners : calibrator->saved_corners()) {
      draw_corners(visualizer, color, corners);
    }
    draw_corners(visualizer, {255, 0, 64}, calibrator->last_corners());
    cv::flip(visualizer, visualizer, 1);
    cv::imshow("Visualization", visualizer);
  }
  cv::destroyAllWindows();
}

void run_camera_orientation(std::vector<CharucoCalibrator>& calibrators) {
  Key key;
  while (key = static_cast<Key>(cv::waitKey(50)), key != Key::ESC) {
    switch (key) {
      case Key::SPACE: {
        for (CharucoCalibrator& calibrator : calibrators) {
          save_camera_parameters(
            calibrator.parameters(),
            get_calibration_path(calibrator.device().camera_id)
          );
        }
        std::cout << "Camera parameters saved." << std::endl;
        break;
      }

      case Key::NONE: break;

      default: {
        std::cout << "Unknown key: " << static_cast<int>(key) << std::endl;
        break;
      }
    }

    // Grab a frame from every camera as fast as possible.
    for (CharucoCalibrator& calibrator : calibrators) {
      calibrator.grab_frame();
    }

    // Detect charuco board and calculate pose, then display.
    for (CharucoCalibrator& calibrator : calibrators) {
      calibrator.detect_board();
      cv::imshow(calibrator.device().device_path, calibrator.frame());
    }
  }
}

int main() {
  std::vector<CharucoCalibrator> calibrators = get_calibrators();
  run_camera_calibration(calibrators);

  for (const CharucoCalibrator& calibrator : calibrators) {
    if (!calibrator.calibrated()) {
      std::cout << "Not all cameras calibrated, exiting." << std::endl;
      return 0;
    }
  }

  std::cout << "Running camera orientation." << std::endl;
  run_camera_orientation(calibrators);
}
