#include "extraction/pose3d.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <span>

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

}
