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
#include "src/tracking.h"

const std::filesystem::path model_dir = "/home/oz/work/ext/openpose/models";

struct Recording {
  std::filesystem::path path;
  std::vector<std::filesystem::path> image_files;
  CameraParameters camera;
  Rectifier rectifier;
};

std::vector<Point> to_points(const op::Array<float>& keypoints, int person_id) {
  std::vector<Point> points;
  for (int point_idx = 0; point_idx < keypoints.getSize(1); ++point_idx) {
    Point& point = points.emplace_back(Point{.point_id = point_idx});
    point.x = keypoints[{person_id, point_idx, 0}];
    point.y = keypoints[{person_id, point_idx, 1}];
    point.confidence = keypoints[{person_id, point_idx, 2}];
  }
  return points;
}

void dump(const std::filesystem::path& filename, const op::Datum& data) {
  std::vector<Person> people;
  for (int i = 0; i < data.poseKeypoints.getSize(0); ++i) {
    Person& person = people.emplace_back();
    person.person_id = i;
    person.body = to_points(data.poseKeypoints, i);
  }
  for (int i = 0; i < data.faceKeypoints.getSize(0); ++i) {
    if (static_cast<std::size_t>(i) > people.size()) {
      people.push_back({.person_id = i});
    }
    people[i].face = to_points(data.faceKeypoints, i);
  }
  for (int i = 0; i < data.handKeypoints[0].getSize(0); ++i) {
    if (static_cast<std::size_t>(i) > people.size()) {
      people.push_back({.person_id = i});
    }
    people[i].left_paw = to_points(data.handKeypoints[0], i);
  }
  for (int i = 0; i < data.handKeypoints[1].getSize(0); ++i) {
    if (static_cast<std::size_t>(i) > people.size()) {
      people.push_back({.person_id = i});
    }
    people[i].right_paw = to_points(data.handKeypoints[1], i);
  }

  save_people(people, filename);;
}

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

  op::WrapperStructHand paw_config;
  paw_config.enable = true; // Enable after upgrading GPU.
  paw_config.detector = op::Detector::BodyWithTracking;
  paw_config.renderMode = op::RenderMode::None;

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
    wrapper.configure(paw_config);
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
        data_file_path.replace_extension(".yml");
        dump(data_file_path, *data->at(0));
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
