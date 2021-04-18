#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <openpose/headers.hpp>
#include <string_view>
#include <vector>

#include "src/timing.h"

double logitech_c920_matrix[] = {
  5.6362511025205447e+02, 0.0, 3.3007199767054630e+02,
  0.0, 5.6362511025205447e+02, 2.4955733002611402e+02,
  0.0, 0.0, 1.0
};
double logitech_c920_distortion[] = {
  0.0, -7.2044862679138386e-02, 0.0, 0.0
};

const std::filesystem::path model_dir = "/home/oz/work/ext/openpose/models";

void dump(
  std::ofstream& out,
  std::string_view part,
  const op::Array<float>& keypoints
) {
  for (int person = 0; person < keypoints.getSize(0); ++person) {
    for (int point = 0; point < keypoints.getSize(1); ++point) {
      out << person << ',' << part << ',' << point;
      for (int xyscore = 0; xyscore < keypoints.getSize(2); ++xyscore) {
        out << ',' << keypoints[{person, point, xyscore}];
      }
      out << '\n';
    }
  }
}

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <path_to_recording>" << std::endl;
    return -1;
  }

  std::filesystem::path recording_dir = argv[1];
  std::vector<std::vector<std::filesystem::path>> image_files;
  std::size_t image_count = 0;
  for (const auto& cam_dir : std::filesystem::directory_iterator{recording_dir}) {
    std::vector<std::filesystem::path>& paths = image_files.emplace_back();
    for (const auto& entry : std::filesystem::directory_iterator{cam_dir}) {
      if (entry.path().extension() == ".png") paths.push_back(entry.path());
    }
    image_count += paths.size();
    std::sort(
      paths.begin(),
      paths.end(),
      [](const std::filesystem::path& a, const std::filesystem::path& b) {
        int a_id =
          std::strtol(a.filename().replace_extension().c_str(), nullptr, 10);
        int b_id =
          std::strtol(b.filename().replace_extension().c_str(), nullptr, 10);
        return a_id < b_id;
      }
    );
  }
  std::size_t digits = 1;
  for (std::size_t i = image_count; i >= 10; i /= 10) ++digits;

  std::cout
    << "Processing " << image_count << " images from " << image_files.size()
    << " cameras." << std::endl;

  cv::Mat distorted_matrix{3, 3, CV_64F, &logitech_c920_matrix};
  cv::Mat distortion{1, 4, CV_64F, &logitech_c920_distortion};
  cv::Mat matrix;
  cv::fisheye::estimateNewCameraMatrixForUndistortRectify(
    distorted_matrix,
    distortion,
    cv::Size{640, 480},
    cv::Matx33d::eye(),
    matrix,
    1
  );

  op::WrapperStructPose pose_config;
  pose_config.modelFolder = model_dir.c_str();
  pose_config.netInputSize = {656, 368};
  pose_config.renderMode = op::RenderMode::None;

  op::WrapperStructFace face_config;
  face_config.enable = true;
  face_config.detector = op::Detector::Body;
  face_config.renderMode = op::RenderMode::None;

  op::WrapperStructHand hand_config;
  // hand_config.enable = true; // Enable after upgrading GPU.
  hand_config.detector = op::Detector::BodyWithTracking;
  hand_config.renderMode = op::RenderMode::None;

  // TODO: Enable these features once out of "experimental."
  op::WrapperStructExtra extra_config;
  // extra_config.identification = true;
  // extra_config.tracking = 0; // Every frame.

  std::size_t processed_count = 0;
  std::size_t tracked_count = 0;
  auto start = steady_clock::now();
  for (const auto& cam_dir : image_files) {
    op::Wrapper wrapper{op::ThreadManagerMode::Asynchronous};
    wrapper.configure(pose_config);
    wrapper.configure(face_config);
    wrapper.configure(hand_config);
    wrapper.configure(extra_config);
    wrapper.start();

    for (const auto& image_path : cam_dir) {
      if (image_path.extension() != ".png") continue;

      op::Matrix image;
      {
        cv::Mat cv_image;
        cv::Mat raw_image = cv::imread(image_path.string());
        cv::fisheye::undistortImage(
          raw_image,
          cv_image,
          distorted_matrix,
          distortion,
          matrix
        );
        image = OP_CV2OPCONSTMAT(cv_image);
      }

      auto data = wrapper.emplaceAndPop(image);
      if (data) {
        ++tracked_count;
        std::filesystem::path data_file_path = image_path;
        data_file_path.replace_extension(".csv");
        std::ofstream data_out{data_file_path};
        data_out << "Person,Part,Point,X,Y,Confidence\n";
        dump(data_out, "Body", data->at(0)->poseKeypoints);
        dump(data_out, "Face", data->at(0)->faceKeypoints);
        data_out.flush();
      }

      if (++processed_count % 50 == 0) {
        auto elapsed = steady_clock::now() - start;
        std::cout
          << std::setw(digits) << tracked_count << " / " << std::setw(digits)
          << processed_count << " of " << image_count << " @ " << std::setw(5)
          << std::setprecision(4) << std::fixed
          << to_fps(processed_count, elapsed) << " fps in "
          << image_path.parent_path() << std::endl;
      }
    }
    wrapper.stop();
  }
}
