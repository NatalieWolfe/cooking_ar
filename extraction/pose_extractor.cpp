#include "extraction/pose_extractor.h"

#include <filesystem>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <openpose/headers.hpp>
#include <span>
#include <vector>

#include "lw/flags/flags.h"

LW_FLAG(
  std::string, openpose_path, "/home/oz/work/ext/openpose",
  "Path to directory where OpenPose was built."
);
LW_FLAG(
  bool, enable_face_pose, true, "Should face poses be extracted from images?"
);
LW_FLAG(
  bool, enable_paw_pose, true, "Should paw poses be extracted from images?"
);

namespace extraction {
namespace {

using ::std::filesystem::path;

const path OPENPOSE_MODEL_DIR = "models";

path openpose_models_path() {
  return path{lw::flags::openpose_path.value()} / OPENPOSE_MODEL_DIR;
}

op::Matrix load_frame(const path& frame_path) {
  cv::Mat frame = cv::imread(frame_path.string());
  return OP_CV2OPCONSTMAT(frame);
}

std::vector<Pose2d::Point> to_points(
  const op::Array<float>& keypoints,
  int person_id
) {
  std::vector<Pose2d::Point> points;
  for (int point_idx = 0; point_idx < keypoints.getSize(1); ++point_idx) {
    points.push_back({
      .point_id = point_idx,
      .x = keypoints[{person_id, point_idx, 0}],
      .y = keypoints[{person_id, point_idx, 1}],
      .confidence = keypoints[{person_id, point_idx, 2}]
    });
  }
  return points;
}

std::vector<Pose2d> data_to_poses(const op::Datum& data) {
  std::vector<Pose2d> poses;
  poses.reserve(data.poseKeypoints.getSize(0));
  for (int i = 0; i < data.poseKeypoints.getSize(0); ++i) {
    Pose2d& pose = poses.emplace_back();
    pose.person_id = i;
    pose.body = to_points(data.poseKeypoints, i);
  }
  for (int i = 0; i < data.faceKeypoints.getSize(0); ++i) {
    if (static_cast<std::size_t>(i) > poses.size()) {
      poses.push_back({.person_id = i});
    }
    poses[i].face = to_points(data.faceKeypoints, i);
  }
  for (int i = 0; i < data.handKeypoints[0].getSize(0); ++i) {
    if (static_cast<std::size_t>(i) > poses.size()) {
      poses.push_back({.person_id = i});
    }
    poses[i].left_paw = to_points(data.handKeypoints[0], i);
  }
  for (int i = 0; i < data.handKeypoints[1].getSize(0); ++i) {
    if (static_cast<std::size_t>(i) > poses.size()) {
      poses.push_back({.person_id = i});
    }
    poses[i].right_paw = to_points(data.handKeypoints[1], i);
  }
  return poses;
}

}

PoseExtractor::PoseExtractor(): _openpose{op::ThreadManagerMode::Asynchronous} {
  op::WrapperStructPose pose_config;
  pose_config.modelFolder = openpose_models_path().c_str();
  pose_config.netInputSize = {656, 368};
  pose_config.renderMode = op::RenderMode::None;

  op::WrapperStructFace face_config;
  face_config.enable = lw::flags::enable_face_pose;
  face_config.detector = op::Detector::Body;
  face_config.renderMode = op::RenderMode::None;

  op::WrapperStructHand paw_config;
  paw_config.enable = lw::flags::enable_paw_pose;
  paw_config.detector = op::Detector::BodyWithTracking;
  paw_config.renderMode = op::RenderMode::None;

  _openpose.configure(pose_config);
  _openpose.configure(face_config);
  _openpose.configure(paw_config);
  _openpose.start();
}

PoseExtractor::~PoseExtractor() {
  _openpose.stop();
}

std::vector<Pose2d> PoseExtractor::get(const path& frame_path) {
  op::Matrix image = load_frame(frame_path);
  auto data = _openpose.emplaceAndPop(image);
  if (!data) return {};
  return data_to_poses(*data->at(0));
}

}
