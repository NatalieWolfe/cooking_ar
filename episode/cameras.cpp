#include "episode/cameras.h"

#include <exception>
#include <filesystem>
#include <fstream>
#include <opencv2/core.hpp>
#include <string>
#include <string_view>

#include "lw/err/canonical.h"
#include "nlohmann/json.hpp"

namespace episode {
namespace {

using ::nlohmann::json;
using ::std::filesystem::path;

constexpr std::string_view CAMERA_BOARD = "OAK-D-LITE";

cv::Mat json_to_matrix(const json& matrix_json) {
  int rows = matrix_json.size();
  cv::Mat matrix;

  if (matrix_json[0].is_array()) {
    int cols = matrix_json[0].size();
    matrix = cv::Mat::zeros(rows, cols, CV_64F);
    for (std::size_t row = 0; row < matrix_json.size(); ++row) {
      for (std::size_t col = 0; col < matrix_json[row].size(); ++col) {
        matrix.at<double>(row, col) = matrix_json[row][col].get<double>();
      }
    }
  } else {
    matrix = cv::Mat::zeros(rows, 1, CV_64F);
    for (std::size_t row = 0; row < matrix_json.size(); ++row) {
      matrix.at<double>(row, 0) = matrix_json[row].get<double>();
    }
  }

  return matrix;
}

cv::Mat json_translation_to_matrix(const json& translation_json) {
  cv::Mat matrix = cv::Mat::zeros(3, 1, CV_64F);
  matrix.at<double>(0, 0) = translation_json["x"].get<double>();
  matrix.at<double>(1, 0) = translation_json["y"].get<double>();
  matrix.at<double>(2, 0) = translation_json["z"].get<double>();
  return matrix;
}

CameraCalibration::Parameters json_to_params(const json& params_json) {
  return {
    .matrix = json_to_matrix(params_json["intrinsicMatrix"]),
    .distortion = json_to_matrix(params_json["distortionCoeff"]),
    .rotation = json_to_matrix(params_json["extrinsics"]["rotationMatrix"]),
    .translation =
      json_translation_to_matrix(params_json["extrinsics"]["translation"]),
  };
}

}

CameraCalibration load_camera_calibration(const path& calibration_path) {
  if (!std::filesystem::exists(calibration_path)) {
    throw lw::NotFound()
      << "Calibration file does not exist at " << calibration_path;
  } else if (calibration_path.extension() != ".json") {
    throw lw::InvalidArgument()
      << "Calibration file is not JSON: " << calibration_path;
  }

  std::ifstream in_stream{calibration_path};
  json calibration_json;
  in_stream >> calibration_json;
  if (!calibration_json.is_object()) {
    throw lw::InvalidArgument()
      << "Invalid calibration, expected an object at " << calibration_path;
  }
  if (calibration_json["boardName"].get<std::string>() != CAMERA_BOARD) {
    throw lw::Unimplemented()
      << "Camera board " << calibration_json["boardName"]
      << " is not supported yet, only " << CAMERA_BOARD;
  }
  if (!calibration_json["cameraData"].is_array()) {
    throw lw::InvalidArgument()
      << "Expected `cameraData` to be an array in " << calibration_path;
  }

  CameraCalibration calibration;
  int left_socket_id =
    calibration_json["stereoRectificationData"]["leftCameraSocket"].get<int>();
  int right_socket_id =
    calibration_json["stereoRectificationData"]["rightCameraSocket"].get<int>();
  for (const json& params_json : calibration_json["cameraData"]) {
    try {
      const int socket_id = params_json[0].get<int>();
      if (socket_id == left_socket_id) {
        calibration.left = json_to_params(params_json[1]);
      } else if (socket_id == right_socket_id) {
        calibration.right = json_to_params(params_json[1]);
      } else {
        calibration.center = json_to_params(params_json[1]);
      }
    } catch (const std::exception& err) {
      throw lw::Internal()
        << "Failed to parse json: " << err.what() << ", json="
        << params_json.dump();
    }
  }
  return calibration;
}

}
