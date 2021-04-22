#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <string>
#include <utility>
#include <vector>

#include "src/cameras.h"
#include "src/files.h"
#include "src/keys.h"
#include "src/tracking.h"

using namespace std::chrono_literals;
using std::filesystem::directory_iterator;

void draw(cv::Mat& image, cv::Scalar color, Point point) {
  // const int radius = 1;
  // const cv::Scalar black{0, 0, 0};
  // color +=
  //   cv::Scalar{255 * std::min(1.0, std::max(0.0, point.confidence)), 0, 0};
  cv::Point center{static_cast<int>(point.x), static_cast<int>(point.y)};
  // cv::circle(image, center, radius + 1, black, cv::FILLED, cv::LINE_AA);
  // cv::circle(image, center, radius, color, cv::FILLED, cv::LINE_AA);

  // cv::addText(image, std::to_string(point.point_id), center, "Hack");
  cv::putText(image, std::to_string(point.point_id), center, cv::FONT_HERSHEY_PLAIN, 0.5, color);
}

void draw(
  cv::Mat& image,
  cv::Scalar color,
  const CameraParameters& camera,
  const std::vector<Point3d>& points
) {
  std::vector<cv::Point3d> points3d;
  std::vector<cv::Point2d> points2d;
  for (const Point3d& point : points) {
    // std::cout << point.x << ',' << point.y << ',' << point.z << std::endl;
    points3d.emplace_back(point.x * 10, point.y * 10, point.z * 10);
  }
  cv::projectPoints(
    points3d,
    camera.rotation,
    camera.translation,
    camera.matrix,
    camera.distortion,
    points2d
  );
  int point_id = 0;
  for (const cv::Point2d& point : points2d) {
    draw(image, color, {.point_id = point_id++, .x = point.x, .y = point.y});
  }
}

void draw(
  cv::Mat& image,
  const CameraParameters& camera,
  const Person3d& person
) {
  draw(image, {0, 0, 0}, camera, person.body);
  draw(image, {0, 64, 0}, camera, person.face);
  draw(image, {0, 128, 0}, camera, person.right_paw);
  draw(image, {0, 196, 0}, camera, person.left_paw);
}

void draw(cv::Mat& image, const Person& person) {
  const cv::Scalar body{0, 0, 0};
  const cv::Scalar face{0, 64, 0};
  const cv::Scalar right_paw{0, 128, 0};
  const cv::Scalar left_paw{0, 192, 0};

  // cv::Scalar color{0, 0, static_cast<double>(person.person_id * 30 % 256)};
  cv::Scalar color{0, 0, 0};
  for (const Point& point : person.body) draw(image, color + body, point);
  for (const Point& point : person.face) draw(image, color + face, point);
  for (const Point& point : person.right_paw) {
    draw(image, color + right_paw, point);
  }
  for (const Point& point : person.left_paw) {
    draw(image, color + left_paw, point);
  }
}

int main() {
  auto recordings_iterator =
    directory_iterator{get_recordings_directory_path()};
  std::map<std::string, CameraParameters> cameras;
  std::vector<std::pair<std::string, std::filesystem::path>> image_files;
  for (const auto& cam_dir : recordings_iterator) {
    const std::string& cam_name = cam_dir.path().stem().string();
    if (cameras.count(cam_name) == 0) {
      cameras[cam_name] = load_camera_parameters(
        get_calibration_path(cam_name)
      );
    }
    for (const auto& file : directory_iterator{cam_dir}) {
      if (file.path().extension() == ".png") {
        image_files.emplace_back(cam_name, file.path());
      }
    }
  }
  std::sort(
    image_files.begin(),
    image_files.end(),
    [](const auto& a_, const auto& b_) {
      const auto& [a_cam, a] = a_;
      const auto& [b_cam, b] = b_;
      std::uint64_t a_id = std::stoull(a.stem().string(), 0, 10) * 1'000'000'000;
      std::uint64_t b_id = std::stoull(b.stem().string(), 0, 10) * 1'000'000'000;
      std::uint64_t a_parent_id = std::stoull(a.parent_path().stem().string(), 0, 10);
      std::uint64_t b_parent_id = std::stoull(b.parent_path().stem().string(), 0, 10);

      return (a_parent_id + a_id) < (b_parent_id + b_id);
    }
  );

  std::size_t i = 0;
  Key key;
  bool use_3d = false;
  while (key = wait_key(33ms), key != Key::ESC) {
    if (key == Key::ONE) ++i;
    else if (key == Key::TWO) i += cameras.size();
    else if (key == Key::THREE) i += 10 * cameras.size();
    else if (key == Key::FOUR) i += 100 * cameras.size();
    else if (key == Key::Q) --i;
    else if (key == Key::W) i -= cameras.size();
    else if (key == Key::E) i -= 10 * cameras.size();
    else if (key == Key::R) i -= 100 * cameras.size();
    else if (key == Key::M) use_3d = !use_3d;
    i %= image_files.size();

    // TODO: Add 3d point tweaking to derive points in space

    auto [cam_name, image_file] = image_files[i];
    cv::Mat image = cv::imread(image_file.string());
    const auto& frame_file = image_file.replace_extension(".yml");
    if (use_3d) {
      std::vector<Person3d> people =
        load_people_3d(get_animation_directory_path() / frame_file.filename());
      for (const Person3d& person : people) {
        draw(image, cameras[cam_name], person);
      }
    } else {
      std::vector<Person> people = load_people(frame_file);
      for (const Person& person : people) draw(image, person);
    }
    cv::putText(image, image_file.string(), {5, 15}, cv::FONT_HERSHEY_PLAIN, 1, {0, 0, 0}, 1, cv::LINE_AA);
    cv::imshow("Visualizer", image);
  }
}
