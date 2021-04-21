#include <algorithm>
#include <filesystem>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <vector>

#include "src/files.h"
#include "src/tracking.h"

using std::filesystem::directory_iterator;

void draw(cv::Mat& image, cv::Scalar color, Point point) {
  const int radius = 1;
  const cv::Scalar black{0, 0, 0};
  color +=
    cv::Scalar{255 * std::min(1.0, std::max(0.0, point.confidence)), 0, 0};
  cv::Point center{static_cast<int>(point.x), static_cast<int>(point.y)};
  cv::circle(image, center, radius + 1, black, cv::FILLED, cv::LINE_AA);
  cv::circle(image, center, radius, color, cv::FILLED, cv::LINE_AA);
}

void draw(cv::Mat& image, const Person& person) {
  // const cv::Scalar body{0, 64, 0};
  // const cv::Scalar face{0, 128, 0};
  // const cv::Scalar right_paw{0, 192, 0};
  // const cv::Scalar left_paw{0, 255, 0};

  cv::Scalar color{0, 0, static_cast<double>(person.person_id * 30 % 256)};
  for (const Point& point : person.body) draw(image, color, point);
  for (const Point& point : person.face) draw(image, color, point);
  for (const Point& point : person.right_paw) {
    draw(image, color, point);
  }
  for (const Point& point : person.left_paw) {
    draw(image, color, point);
  }
}

int main() {
  auto recordings_iterator =
    directory_iterator{get_recordings_directory_path()};
  int cam_count = 0;
  std::vector<std::filesystem::path> image_files;
  for (const auto& cam_dir : recordings_iterator) {
    ++cam_count;
    for (const auto& file : directory_iterator{cam_dir}) {
      if (file.path().extension() == ".png") image_files.push_back(file.path());
    }
  }
  std::sort(
    image_files.begin(),
    image_files.end(),
    [](const std::filesystem::path& a, const std::filesystem::path& b) {
      std::uint64_t a_id = std::stoull(a.stem().string(), 0, 10) * 1'000'000'000;
      std::uint64_t b_id = std::stoull(b.stem().string(), 0, 10) * 1'000'000'000;
      std::uint64_t a_parent_id = std::stoull(a.parent_path().stem().string(), 0, 10);
      std::uint64_t b_parent_id = std::stoull(b.parent_path().stem().string(), 0, 10);

      return (a_parent_id + a_id) < (b_parent_id + b_id);
    }
  );

  std::size_t i = 0;
  int key = -1;
  while (key = cv::waitKey(33), key != 27) {
    if (key == 49) ++i; // 1
    else if (key == 50) i += cam_count; // 2
    else if (key == 51) i += 10 * cam_count; // 3
    else if (key == 52) i += 100 * cam_count; // 4
    else if (key == 113) --i; // q
    else if (key == 119) i -= cam_count; // w
    else if (key == 101) i -= 10 * cam_count; // e
    else if (key == 114) i -= 100 * cam_count; // r
    i %= image_files.size();

    std::filesystem::path image_file = image_files[i];
    cv::Mat image = cv::imread(image_file.string());
    std::vector<Person> people =
      load_people(image_file.replace_extension(".yml"));
    for (const Person& person : people) draw(image, person);
    cv::imshow("Visualizer", image);
  }
}
