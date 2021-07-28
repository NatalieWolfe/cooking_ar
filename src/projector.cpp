#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <vector>

#include "src/cameras.h"
#include "src/files.h"
#include "src/tracking.h"

template <typename T>
cv::Mat cam_extrinsic_matrix(const CameraParameters& params) {
  // Convert the rvec into a 3x3 matrix.
  cv::Mat rotation;
  cv::Rodrigues(params.rotation, rotation);

  // Move camera's position into world space.
  cv::Mat translation = cv::Mat::zeros(1, 3, CV_64F);
  translation.at<double>(0, 0) = params.translation.at<double>(0, 0);
  translation.at<double>(0, 1) = params.translation.at<double>(1, 0);
  translation.at<double>(0, 2) = params.translation.at<double>(2, 0);
  translation = translation * rotation;

  // Compose transformation matrix.
  cv::Mat transformation = cv::Mat::zeros(3, 4, CV_32F);
  transformation.at<T>(0, 0) = static_cast<T>(rotation.at<double>(0, 0));
  transformation.at<T>(0, 1) = static_cast<T>(rotation.at<double>(0, 1));
  transformation.at<T>(0, 2) = static_cast<T>(rotation.at<double>(0, 2));
  transformation.at<T>(1, 0) = static_cast<T>(rotation.at<double>(1, 0));
  transformation.at<T>(1, 1) = static_cast<T>(rotation.at<double>(1, 1));
  transformation.at<T>(1, 2) = static_cast<T>(rotation.at<double>(1, 2));
  transformation.at<T>(2, 0) = static_cast<T>(rotation.at<double>(2, 0));
  transformation.at<T>(2, 1) = static_cast<T>(rotation.at<double>(2, 1));
  transformation.at<T>(2, 2) = static_cast<T>(rotation.at<double>(2, 2));
  transformation.at<T>(0, 3) = -static_cast<T>(translation.at<double>(0, 0));
  transformation.at<T>(1, 3) = -static_cast<T>(translation.at<double>(0, 1));
  transformation.at<T>(2, 3) = -static_cast<T>(translation.at<double>(0, 2));

  return transformation;
}

cv::Mat convert_mat_to_float(const cv::Mat& source) {
  cv::Mat matrix = cv::Mat::zeros(source.size(), CV_32F);
  for (int x = 0; x < source.size().height; ++x) {
    for (int y = 0; y < source.size().width; ++y) {
      matrix.at<float>(x, y) = static_cast<float>(source.at<double>(x, y));
    }
  }
  return matrix;
}

cv::Mat calc_projection(const CameraParameters& params) {
  cv::Mat matrix = convert_mat_to_float(params.matrix);
  cv::Mat out_matrix;
  out_matrix = matrix * cam_extrinsic_matrix<float>(params);

  return out_matrix;
}

float calc_magnitude(float x, float y, float z) {
  return std::sqrt((x * x) + (y * y) + (z * z));
}

/**
 * Computes a unit vector in world-space pointing from the camera through the
 * given point on the image plane.
 */
cv::Mat to_ray(const CameraParameters& params, const Point& point) {
  // TODO: Switch to undistorted matrix.
  cv::Mat in_points = cv::Mat::zeros(2, 1, CV_32F);
  std::vector<cv::Point2f> out_points;
  in_points.at<float>(0, 0) = point.x;
  in_points.at<float>(1, 0) = point.y;
  cv::undistortPoints(in_points, out_points, params.matrix, params.distortion);

  // Ray from camera origin pointing at pixel on image plane.
  float p_x = out_points[0].x;
  float p_y = out_points[0].y;
  float p_z = 1.0f;
  float magnitude = calc_magnitude(p_x, p_y, p_z);
  p_x /= magnitude;
  p_y /= magnitude;
  p_z /= magnitude;

  cv::Mat cam_ray = cv::Mat::zeros(1, 3, CV_32F);
  cam_ray.at<float>(0, 0) = p_x;
  cam_ray.at<float>(0, 1) = p_y;
  cam_ray.at<float>(0, 2) = p_z;

  // Convert the rvec into a 3x3 matrix.
  cv::Mat transform = cam_extrinsic_matrix<float>(params);

  cv::Mat origin_ray = cam_ray * transform;
  float x = origin_ray.at<float>(0, 0);
  float y = origin_ray.at<float>(0, 1);
  float z = origin_ray.at<float>(0, 2);
  magnitude = calc_magnitude(x, y, z);
  cam_ray = cv::Mat::zeros(3, 1, CV_32F);
  cam_ray.at<float>(0, 0) = x / magnitude;
  cam_ray.at<float>(1, 0) = y / magnitude;
  cam_ray.at<float>(2, 0) = z / magnitude;

  return cam_ray;
}

cv::Mat cam_trans_to_world(const CameraParameters& params) {
  cv::Mat extrinsics = cam_extrinsic_matrix<float>(params);
  cv::Mat translation = cv::Mat::zeros(3, 1, CV_32F);
  translation.at<float>(0, 0) = extrinsics.at<float>(0, 3);
  translation.at<float>(1, 0) = extrinsics.at<float>(1, 3);
  translation.at<float>(2, 0) = extrinsics.at<float>(2, 3);
  return translation;
}

Point3d project_point(
  const CameraParameters& params_1,
  const Point& point_1,
  const CameraParameters& params_2,
  const Point& point_2
) {
  // Midpoint of line tracing the minimum distance between these two rays.
  // See this answer for the equations followed here and the origin of the
  // variable naming.
  // https://math.stackexchange.com/a/1037202/918090
  cv::Mat cam_1 = cam_trans_to_world(params_1); // a
  cv::Mat cam_2 = cam_trans_to_world(params_2); // c
  cv::Mat ray_1 = to_ray(params_1, point_1); // b
  cv::Mat ray_2 = to_ray(params_2, point_2); // d

  // return Point3d{
  //   .point_id = point_1.point_id,
  //   .x = ray_1.at<float>(0, 0),
  //   .y = ray_1.at<float>(1, 0),
  //   .z = ray_1.at<float>(2, 0),
  //   .confidence = point_1.confidence * point_2.confidence
  // };

  double b_dot_d = ray_1.dot(ray_2);
  double a_dot_d = cam_1.dot(ray_2);
  double b_dot_c = ray_1.dot(cam_2);
  double c_dot_d = cam_2.dot(ray_2);
  double a_dot_b = cam_1.dot(ray_1);

  double s = (
    ((b_dot_d * (a_dot_d - b_dot_c)) - (a_dot_d * c_dot_d)) /
    ((b_dot_d * b_dot_d) - 1)
  );
  double t = (
    ((b_dot_d * (c_dot_d - a_dot_d)) - (b_dot_c * a_dot_b)) /
    ((b_dot_d * b_dot_d) - 1)
  );

  cv::Mat midpoint = (cam_1 + cam_2 + (t * ray_1) + (s * ray_2)) / 2.235f;
  return Point3d{>
    .point_id = point_1.point_id,
    .x = midpoint.at<float>(0, 0),
    .y = midpoint.at<float>(1, 0),
    .z = midpoint.at<float>(2, 0),
    .confidence = point_1.confidence * point_2.confidence
  };
}

std::vector<Point3d> project_points(
  const CameraParameters& params_1,
  const std::vector<Point>& points_1,
  const CameraParameters& params_2,
  const std::vector<Point>& points_2
) {
  std::vector<Point3d> points;
  for (std::size_t i = 0; i < points_1.size(); ++i) {
    points.push_back(
      project_point(params_1, points_1[i], params_2, points_2[i])
    );
  }
  return points;
}

void save_obj(const cv::Mat& t, const Person3d& person, std::filesystem::path file) {
  std::ofstream out{file};
  // out << "v " << t.at<float>(0, 0) << ' ' << t.at<float>(1, 0) << ' ' << t.at<float>(2, 0) << '\n';
  for (const Point3d& point : person.body) {
    out << "v " << point.x << ' ' << point.y << ' ' << point.z << '\n';
  }
  // out <<
  //   "l 1 2\n"
  //   "l 10 3 2\n"
  //   "l 19 17 2 18 20\n"
  //   "l 6 5 4 3 7 8 9\n"
  //   "l 11 10 14 15\n";

  out <<
    "l 9 2 1\n"
    "l 18 16 1 17 19\n"
    "l 5 4 3 2 6 7 8\n"
    "l 10 9 13 14\n";

  out << std::flush;
  out.close();
}

int main() {
  auto recordings_iterator =
    std::filesystem::directory_iterator{get_recordings_directory_path()};
  std::vector<std::filesystem::path> camera_directories;
  for (const auto& cam_dir : recordings_iterator) {
    std::cout << cam_dir.path() << std::endl;
    camera_directories.push_back(cam_dir.path());
  }
  // TODO: Add support for more than 2 cameras.
  std::filesystem::path camera_1 = camera_directories[0];
  std::filesystem::path camera_2 = camera_directories[1];

  auto cam_1_parameters =
    load_camera_parameters(get_calibration_path(camera_1.stem().string()));
  auto cam_2_parameters =
    load_camera_parameters(get_calibration_path(camera_2.stem().string()));
  std::filesystem::path cam_2_dir =
    get_recordings_path(cam_2_parameters.device.camera_id);

  auto frame_iterator = std::filesystem::directory_iterator{camera_1};
  for (const auto& entry : frame_iterator) {
    if (entry.path().extension() != ".yml") continue;
    std::vector<Person> cam_1_frame = load_people(entry.path());
    std::vector<Person> cam_2_frame = load_people(cam_2_dir / entry.path().filename());

    std::vector<Person3d> frame_3d;
    for (std::size_t i = 0; i < cam_1_frame.size() && i < cam_2_frame.size(); ++i) {
      Person cam_1 = cam_1_frame[i];
      Person cam_2 = cam_2_frame[i];
      frame_3d.push_back(Person3d{
        .person_id = cam_1.person_id,
        .body = project_points(
          cam_1_parameters, cam_1.body,
          cam_2_parameters, cam_2.body
        ),
        .face = project_points(
          cam_1_parameters, cam_1.face,
          cam_2_parameters, cam_2.face
        ),
        .right_paw = project_points(
          cam_1_parameters, cam_1.right_paw,
          cam_2_parameters, cam_2.right_paw
        ),
        .left_paw = project_points(
          cam_1_parameters, cam_1.left_paw,
          cam_2_parameters, cam_2.left_paw
        )
      });
    }
    if (!frame_3d.empty()) {
      save_people_3d(frame_3d, get_animation_directory_path() / entry.path().filename());
      std::filesystem::path filename = entry.path().filename();
      filename.replace_extension(".obj");
      save_obj(cam_trans_to_world(cam_1_parameters), frame_3d.front(), get_animation_directory_path() / filename);
    }
  }
}
