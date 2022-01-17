#pragma once

#include <filesystem>
#include <span>
#include <vector>

namespace extraction {

struct Pose3d {
  struct Point {
    int point_id;
    double x;
    double y;
    double z;
    double confidence;
  };

  int person_id;
  std::vector<Point> body;
  std::vector<Point> face;
  std::vector<Point> left_paw;
  std::vector<Point> right_paw;
};

void write_poses(
  const std::filesystem::path& out_file,
  std::span<const Pose3d> poses
);

}
