#pragma once

#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/core.hpp>
#include <string>
#include <string_view>
#include <vector>

namespace recording {

// Returns the ChAruCo board used for calibration.
cv::Ptr<cv::aruco::CharucoBoard> get_charuco_board() {
  static cv::Ptr<cv::aruco::CharucoBoard> board =
    cv::aruco::CharucoBoard::create(
      /*squaresX=*/5, /*squaresY=*/7,
      /*squareLength=*/0.0303f, /*markerLength*/0.01515f,
      cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250)
    );
  return board;
}

struct CameraCalibration {
  cv::Mat matrix;       // Projection matrix.
  cv::Mat distortion;   // Image distortion coefficients.
  cv::Mat rotation;     // Rodriguez rotation vector.
  cv::Mat translation;  // Translation vector.
};

class CharucoCalibrator {
public:
  explicit CharucoCalibrator(cv::Ptr<cv::aruco::CharucoBoard> board):
    _board{board}
  {}

  // Replaces the latest frame with the one given and attempts to detect the
  // board.
  void set_latest_frame(cv::Mat frame);
  // Saves the board information from the current frame and attempts
  // calibration.
  void save_latest_frame();
  // Drops the most recent saved frame and attempts to recalibrate.
  void drop_latest_frame();

  // Returns true if possible calibration data has been generated.
  bool calibrated() const {
    return !_calibration.matrix.empty() && !_calibration.distortion.empty();
  }
  // Returns the calculated calibration data.
  const CameraCalibration& calibration() const { return _calibration; }

  // Returns the last frame that was added.
  const cv::Mat& latest_frame() const { return _frame; }
  // Attempts to undo lense distoration from the camera on the latest frame.
  cv::Mat undistort_latest_frame() const;

  // Draws the detected corners from the latest frame onto the given image.
  void draw_latest_corners(cv::InputOutputArray image, cv::Scalar color) const;
  // Draws all the saved corners onto the given frame.
  void draw_saved_corners(cv::InputOutputArray image, cv::Scalar color) const;

  // Returns the debug string generated while processing the latest frame.
  std::string_view debug_text() const { return _debug_text; }

private:
  // Calculates the camera projection matrix and distortion coefficients, saving
  // them to the calibration struct.
  void _calibrate();

  // Estimates the pose of the charuco board and saves the results into the
  // calibration struct.
  bool _estimate_pose();

  cv::Ptr<cv::aruco::CharucoBoard> _board;
  cv::Mat _frame;
  cv::Mat _last_corners;
  cv::Mat _last_corner_ids;
  std::vector<cv::Mat> _saved_corners;
  std::vector<cv::Mat> _saved_corner_ids;
  double _error_rate = 420.69;
  std::string _debug_text;
  CameraCalibration _calibration;
};

}
