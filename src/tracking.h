#pragma once

#include <filesystem>
#include <vector>

struct Point {
  int point_id;
  double x;
  double y;
  double confidence;
};

struct Point3d {
  int point_id;
  double x;
  double y;
  double z;
  double confidence;
};

struct Person {
  int person_id;
  std::vector<Point> body;
  std::vector<Point> face;
  std::vector<Point> right_paw;
  std::vector<Point> left_paw;
};

struct Person3d {
  int person_id;
  std::vector<Point3d> body;
  std::vector<Point3d> face;
  std::vector<Point3d> right_paw;
  std::vector<Point3d> left_paw;
};

void save_people(
  const std::vector<Person>& people,
  const std::filesystem::path& filename
);

void save_people_3d(
  const std::vector<Person3d>& people,
  const std::filesystem::path& filename
);

std::vector<Person> load_people(const std::filesystem::path& filename);
