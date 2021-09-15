#include "recording/monitor/charuco.h"

#include <opencv2/aruco.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <sstream>
#include <string>
#include <vector>

namespace recording {
namespace {

constexpr std::size_t MIN_CALIBRATION_FRAMES = 5;

std::string dump(const cv::Mat& matrix) {
  std::stringstream stream;
  stream << matrix;
  return stream.str();
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

}

cv::Ptr<cv::aruco::CharucoBoard> get_charuco_board() {
  static cv::Ptr<cv::aruco::CharucoBoard> board =
    cv::aruco::CharucoBoard::create(
      /*squaresX=*/5, /*squaresY=*/7,
      /*squareLength=*/0.0303f, /*markerLength*/0.01515f,
      cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250)
    );
  return board;
}

void CharucoCalibrator::set_latest_frame(cv::Mat frame) {
  frame.copyTo(_frame);
  // _debug_text = _parameters.device.device_path.string() + '\n';
  _debug_text = "RMS: " + std::to_string(_error_rate) + '\n';

  // Initialize detection parameters.
  cv::Ptr<cv::aruco::DetectorParameters> params =
    cv::aruco::DetectorParameters::create();
  params->cornerRefinementMethod = cv::aruco::CORNER_REFINE_CONTOUR;

  // Detect the AruCo markers on the board.
  std::vector<int> marker_ids;
  std::vector<std::vector<cv::Point2f>> marker_corners;
  cv::aruco::detectMarkers(
    _frame,
    _board->dictionary,
    marker_corners,
    marker_ids,
    params
  );
  if (marker_ids.empty()) return;

  // Find the ChAruCo markers interleaved on the board.
  cv::aruco::interpolateCornersCharuco(
    marker_corners,
    marker_ids,
    _frame,
    _board,
    _last_corners,
    _last_corner_ids
  );
  if (_last_corner_ids.empty() || !calibrated()) return;

  // Determine the camera's position relative to the board.
  if (_estimate_pose()) {
    _debug_text += "tvec: " + dump(_calibration.translation) + '\n';
    _debug_text += "rvec: " + dump(_calibration.rotation) + '\n';
  }
}

void CharucoCalibrator::save_latest_frame() {
  if (!_last_corners.empty()) {
    _saved_corners.push_back(_last_corners);
    _saved_corner_ids.push_back(_last_corner_ids);
    if (_saved_corners.size() >= MIN_CALIBRATION_FRAMES) _calibrate();
  }
}

void CharucoCalibrator::drop_latest_frame() {
  if (!_saved_corners.empty()) {
    _saved_corners.pop_back();
    _saved_corner_ids.pop_back();
    if (_saved_corners.size() >= MIN_CALIBRATION_FRAMES) _calibrate();
  }
}

cv::Mat CharucoCalibrator::undistort_latest_frame() const {
  cv::Mat undistorted_map_1;
  cv::Mat undistorted_map_2;
  cv::Mat optimal_matrix = cv::getOptimalNewCameraMatrix(
    _calibration.matrix,
    _calibration.distortion,
    _frame.size(),
    /*alpha=*/0.0,
    _frame.size()
  );
  cv::initUndistortRectifyMap(
    _calibration.matrix,
    _calibration.distortion,
    /*R=*/cv::noArray(),
    optimal_matrix,
    _frame.size(),
    CV_16SC2,
    undistorted_map_1,
    undistorted_map_2
  );

  cv::Mat output;
  cv::remap(
    _frame,
    output,
    undistorted_map_1,
    undistorted_map_2,
    cv::INTER_CUBIC
  );
  return output;
}

void CharucoCalibrator::draw_latest_corners(
  cv::InputOutputArray image,
  cv::Scalar color
) const {
  draw_corners(image, color, _last_corners);
}

void CharucoCalibrator::draw_saved_corners(
  cv::InputOutputArray image,
  cv::Scalar color
) const {
  for (const cv::Mat& corners : _saved_corners) {
    draw_corners(image, color, corners);
  }
}

void CharucoCalibrator::_calibrate() {
  _error_rate = cv::aruco::calibrateCameraCharuco(
    _saved_corners, _saved_corner_ids, _board, _frame.size(),
    _calibration.matrix, _calibration.distortion,
    _calibration.rotation, _calibration.translation,
    /*stdDeviationsIntrinsics=*/cv::noArray(),
    /*stdDeviationsExtrinsics=*/cv::noArray(),
    /*perViewErrors=*/cv::noArray()
  );
}

bool CharucoCalibrator::_estimate_pose() {
  return cv::aruco::estimatePoseCharucoBoard(
    _last_corners,
    _last_corner_ids,
    _board,
    _calibration.matrix,
    _calibration.distortion,
    _calibration.rotation,
    _calibration.translation
  );
}

}
