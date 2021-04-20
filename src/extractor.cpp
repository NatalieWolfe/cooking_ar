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

#include "src/cameras.h"
#include "src/files.h"
#include "src/timing.h"

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

struct Recording {
  std::filesystem::path path;
  std::vector<std::filesystem::path> image_files;
  CameraParameters camera;
  Rectifier rectifier;
};

int main() {
  std::vector<Recording> recordings;
  std::size_t image_count = 0;
  auto recordings_iterator =
    std::filesystem::directory_iterator{get_recordings_directory_path()};
  for (const auto& cam_dir : recordings_iterator) {
    Recording& recording = recordings.emplace_back(Recording{.path = cam_dir});
    recording.camera = load_camera_parameters(
      get_calibration_path(cam_dir.path().stem().string())
    );

    for (const auto& entry : std::filesystem::directory_iterator{cam_dir}) {
      if (entry.path().extension() == ".png") {
        recording.image_files.push_back(entry.path());
      }
    }

    recording.rectifier = Rectifier{
      recording.camera,
      cv::imread(recording.image_files.front().string()).size()
    };

    image_count += recording.image_files.size();
    std::sort(
      recording.image_files.begin(),
      recording.image_files.end(),
      [](const std::filesystem::path& a, const std::filesystem::path& b) {
        int a_id = std::stoi(a.stem().string(), 0, 10);
        int b_id = std::stoi(b.stem().string(), 0, 10);
        return a_id < b_id;
      }
    );
  }
  std::size_t digits = 1;
  for (std::size_t i = image_count; i >= 10; i /= 10) ++digits;

  std::cout
    << "Processing " << image_count << " images from " << recordings.size()
    << " cameras." << std::endl;

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
  for (const Recording& recording : recordings) {
    op::Wrapper wrapper{op::ThreadManagerMode::Asynchronous};
    wrapper.configure(pose_config);
    wrapper.configure(face_config);
    wrapper.configure(hand_config);
    wrapper.configure(extra_config);
    wrapper.start();

    for (const auto& image_path : recording.image_files) {
      op::Matrix image;
      {
        cv::Mat raw_image = cv::imread(image_path.string());
        cv::Mat cv_image = recording.rectifier(raw_image);
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

      if (++processed_count % 100 == 0) {
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
  std::cout
    << "All frames processed in " << to_hms(steady_clock::now() - start)
    << std::endl;
}
