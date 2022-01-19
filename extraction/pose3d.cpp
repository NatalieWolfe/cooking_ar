#include "extraction/pose3d.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <span>

#include "lw/err/canonical.h"
#include "nlohmann/json.hpp"

namespace extraction {
namespace {

using ::nlohmann::json;
using ::std::filesystem::path;

json point_to_json(const Pose3d::Point& point) {
  json json_point;
  json_point.push_back(point.point_id);
  json_point.push_back(point.x);
  json_point.push_back(point.y);
  json_point.push_back(point.z);
  json_point.push_back(point.confidence);
  return json_point;
}

json points_to_json(std::span<const Pose3d::Point> points) {
  json json_points;
  std::transform(
    points.begin(),
    points.end(),
    std::back_inserter(json_points),
    &point_to_json
  );
  return json_points;
}

json pose_to_json(const Pose3d& pose) {
  json json_pose;
  json_pose["person_id"] = pose.person_id;
  json_pose["body"] = points_to_json(pose.body);
  json_pose["face"] = points_to_json(pose.face);
  json_pose["left_paw"] = points_to_json(pose.left_paw);
  json_pose["right_paw"] = points_to_json(pose.right_paw);
  return json_pose;
}

Pose3d::Point json_to_point(const json& json_point) {
  if (!json_point.is_array() || json_point.size() != 5) {
    throw lw::InvalidArgument()
      << "Invalid point, expected array with 5 elements got: " << json_point;
  }
  return {
    .point_id = json_point[0].get<int>(),
    .x = json_point[1].get<double>(),
    .y = json_point[2].get<double>(),
    .z = json_point[3].get<double>(),
    .confidence = json_point[4].get<double>()
  };
}

std::vector<Pose3d::Point> json_to_points(
  const json& json_pose,
  const std::string& key
) {
  const json& json_points = json_pose[key];
  if (!json_points.is_array()) {
    throw lw::InvalidArgument()
      << "Pose " << key << " must be an array of points.";
  }
  std::vector<Pose3d::Point> points;
  points.reserve(json_points.size());
  std::transform(
    json_points.begin(),
    json_points.end(),
    std::back_inserter(points),
    &json_to_point
  );
  return points;
}

Pose3d json_to_pose(const json& json_pose) {
  if (!json_pose["person_id"].is_number_integer()) {
    throw lw::InvalidArgument() << "Pose person_id must be an integer.";
  }
  return {
    .person_id = json_pose["person_id"].get<int>(),
    .body = json_to_points(json_pose, "body"),
    .face = json_to_points(json_pose, "face"),
    .left_paw = json_to_points(json_pose, "left_paw"),
    .right_paw = json_to_points(json_pose, "right_paw"),
  };
}

}

void write_poses(const path& out_file, std::span<const Pose3d> poses) {
  json out;
  std::transform(
    poses.begin(),
    poses.end(),
    std::back_inserter(out),
    &pose_to_json
  );

  std::ofstream out_stream{out_file};
  out_stream << std::setw(2) << out << std::endl;
}

std::vector<Pose3d> read_poses3d(const path& pose_file) {
  if (!std::filesystem::exists(pose_file)) {
    throw lw::NotFound() << "Pose file does not exist at " << pose_file;
  } else if (pose_file.extension() != ".json") {
    throw lw::InvalidArgument() << "Pose file is not JSON: " << pose_file;
  }

  std::ifstream in_stream{pose_file};
  json poses_json;
  in_stream >> poses_json;
  if (poses_json.is_null()) return {};
  if (!poses_json.is_array()) {
    throw lw::InvalidArgument() << "Pose file is illformed: " << pose_file;
  }

  std::vector<Pose3d> poses;
  poses.reserve(poses_json.size());
  try {
    std::transform(
      poses_json.begin(),
      poses_json.end(),
      std::back_inserter(poses),
      &json_to_pose
    );
  } catch (const lw::Error& err) {
    throw lw::InvalidArgument() << err.what() << " in file " << pose_file;
  }
  return poses;
}

}
