#include "src/tracking.h"

#include <filesystem>
#include <opencv2/core.hpp>
#include <vector>

namespace {
void write(cv::FileStorage& file, const Point& point) {
  file.write("point_id", point.point_id);
  file.write("x", point.x);
  file.write("y", point.y);
  file.write("confidence", point.confidence);
}

void write(cv::FileStorage& file, const std::vector<Point>& points) {
  for (const Point& point : points) {
    file.startWriteStruct("", cv::FileNode::MAP, "Point");
    write(file, point);
    file.endWriteStruct();
  }
}

void write(cv::FileStorage& file, const Person& person) {
  file.write("person_id", person.person_id);
  file.startWriteStruct("body", cv::FileNode::SEQ);
  write(file, person.body);
  file.endWriteStruct();
  file.startWriteStruct("face", cv::FileNode::SEQ);
  write(file, person.face);
  file.endWriteStruct();
  file.startWriteStruct("right_paw", cv::FileNode::SEQ);
  write(file, person.right_paw);
  file.endWriteStruct();
  file.startWriteStruct("left_paw", cv::FileNode::SEQ);
  write(file, person.left_paw);
  file.endWriteStruct();
}

void write(cv::FileStorage& file, const Point3d& point) {
  file.write("point_id", point.point_id);
  file.write("x", point.x);
  file.write("y", point.y);
  file.write("z", point.z);
  file.write("confidence", point.confidence);
}

void write(cv::FileStorage& file, const std::vector<Point3d>& points) {
  for (const Point3d& point : points) {
    file.startWriteStruct("", cv::FileNode::MAP, "Point3d");
    write(file, point);
    file.endWriteStruct();
  }
}

void write(cv::FileStorage& file, const Person3d& person) {
  file.write("person_id", person.person_id);
  file.startWriteStruct("body", cv::FileNode::SEQ);
  write(file, person.body);
  file.endWriteStruct();
  file.startWriteStruct("face", cv::FileNode::SEQ);
  write(file, person.face);
  file.endWriteStruct();
  file.startWriteStruct("right_paw", cv::FileNode::SEQ);
  write(file, person.right_paw);
  file.endWriteStruct();
  file.startWriteStruct("left_paw", cv::FileNode::SEQ);
  write(file, person.left_paw);
  file.endWriteStruct();
}

Point read_point(const cv::FileNode& file) {
  return Point {
    .point_id = static_cast<int>(file["point_id"]),
    .x = static_cast<double>(file["x"]),
    .y = static_cast<double>(file["y"]),
    .confidence = static_cast<double>(file["confidence"]),
  };
}

std::vector<Point> read_points(const cv::FileNode& file) {
  std::vector<Point> points;
  for (const cv::FileNode& point : file) points.push_back(read_point(point));
  return points;
}

Person read_person(const cv::FileNode& file) {
  return Person{
    .person_id = static_cast<int>(file["person_id"]),
    .body{read_points(file["body"])},
    .face{read_points(file["face"])},
    .right_paw{read_points(file["right_paw"])},
    .left_paw{read_points(file["left_paw"])}
  };
}

std::vector<Person> read_people(const cv::FileNode& file) {
  std::vector<Person> people;
  for (const cv::FileNode& person : file) people.push_back(read_person(person));
  return people;
}

}

void save_people(
  const std::vector<Person>& people,
  const std::filesystem::path& filename
) {
  cv::FileStorage file{
    filename.string(),
    cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML
  };
  file.startWriteStruct("people", cv::FileNode::SEQ);
  for (const Person& person : people) {
    file.startWriteStruct("", cv::FileNode::MAP, "Person");
    write(file, person);
    file.endWriteStruct();
  }
  file.endWriteStruct();
  file.release();
}

void save_people_3d(
  const std::vector<Person3d>& people,
  const std::filesystem::path& filename
) {
  cv::FileStorage file{
    filename.string(),
    cv::FileStorage::WRITE | cv::FileStorage::FORMAT_YAML
  };
  file.startWriteStruct("people", cv::FileNode::SEQ);
  for (const Person3d& person : people) {
    file.startWriteStruct("", cv::FileNode::MAP, "Person3d");
    write(file, person);
    file.endWriteStruct();
  }
  file.endWriteStruct();
  file.release();
}

std::vector<Person> load_people(const std::filesystem::path& filename) {
  cv::FileStorage file{
    filename.string(),
    cv::FileStorage::READ | cv::FileStorage::FORMAT_YAML
  };
  return read_people(file["people"]);
}
