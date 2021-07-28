#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <functional>
#include <memory>
#include <mutex>
#include <opencv2/aruco.hpp>
#include <opencv2/aruco/charuco.hpp>
#include <opencv2/calib3d.hpp>
#include <opencv2/core.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <optional>
#include <string>
#include <thread>
#include <vector>

double cheap_cam_matrix[] = {
  8.0637614687967880e+02, 0.0, 3.0520590183528248e+02,
  0.0, 8.0637614687967880e+02, 2.6662585346235818e+02,
  0.0, 0.0, 1.0
};
double cheap_cam_distortion[] = {
  -4.8717081999920098e-01, 0.0, -1.8695894697204840e-02,
  8.6171180427094835e-03, 0.0
};

double logitech_c920_matrix[] = {
  1.4611308193324010e+03, 0.0, 9.6725501506486341e+02,
  0.0, 1.4611308193324010e+03, 5.5545825804372771e+02,
  0.0, 0.0, 1.0
};
double logitech_c920_distortion[] = {
  4.2761294057302876e-02, -1.8215867310222439e-01,
  0.0, 0.0, 1.2308737273214458e-01
};

struct CameraDetails {
  cv::Mat matrix;
  cv::Mat distortion;
  cv::Size image_size;
  cv::Vec3d rotation;
  cv::Vec3d translation;
  cv::Mat undistorted_matrix;
};

class FrameProcessor {
public:
  explicit FrameProcessor(int camera_id):
    _camera_id{camera_id},
    _camera{
      .matrix{3, 3, CV_64F, &logitech_c920_matrix},
      .distortion{1, 5, CV_64F, &logitech_c920_distortion},
      .image_size{1920, 1080}
    },
    _ready{false}
  {}

  void start() {
    std::cout << _camera_id << ": start." << std::endl;

    _camera_input.open(_camera_id);
    _camera_input.set(cv::CAP_PROP_FRAME_WIDTH, _camera.image_size.width);
    _camera_input.set(cv::CAP_PROP_FRAME_HEIGHT, _camera.image_size.height);
    while (cv::waitKey(10) != 27) {
      _calibrate_camera_position();
    }
    cv::fisheye::estimateNewCameraMatrixForUndistortRectify(
      _camera.matrix,
      _camera.distortion,
      _camera.image_size,
      cv::Matx33d::eye(),
      _camera.undistorted_matrix,
      1
    );
    _thread = std::thread{
      std::bind(&FrameProcessor::_process_frame_loop, this)
    };
  }

  void tick() {
    std::cout << _camera_id << ": tick." << std::endl;
    _ready = true;
    _ready_flag.notify_all();
  }

  void wait() {
    std::cout << _camera_id << ": wait." << std::endl;
    std::unique_lock<std::mutex> lock{_mutex};
    _ready_flag.wait(lock, [this]() { return _ready.load() == false; });
  }

private:
  double f_x() const { return logitech_c920_matrix[0]; }
  double f_y() const { return logitech_c920_matrix[4]; }
  double c_x() const { return logitech_c920_matrix[2]; }
  double c_y() const { return logitech_c920_matrix[5]; }

  bool _calibrate_camera_position() {
    std::string view_name = "calibrating cam " + std::to_string(_camera_id);

    cv::Ptr<cv::aruco::Dictionary> dictionary =
      cv::aruco::getPredefinedDictionary(cv::aruco::DICT_6X6_250);
    cv::Ptr<cv::aruco::CharucoBoard> board =
      cv::aruco::CharucoBoard::create(5, 7, 0.03f, 0.015f, dictionary);
    cv::Ptr<cv::aruco::DetectorParameters> params =
      cv::aruco::DetectorParameters::create();

    cv::Mat image = _capture();
    cv::imshow(view_name, image);
    cv::Mat image_copy;
    image.copyTo(image_copy);

    std::vector<int> marker_ids;
    std::vector<std::vector<cv::Point2f>> marker_corners;
    cv::aruco::detectMarkers(
      image,
      board->dictionary,
      marker_corners,
      marker_ids,
      params
    );
    if (marker_ids.empty()) return false;

    std::vector<int> charuco_ids;
    std::vector<cv::Point2f> charuco_corners;
    if (
      cv::aruco::interpolateCornersCharuco(
        marker_corners,
        marker_ids,
        image,
        board,
        charuco_corners,
        charuco_ids,
        _camera.matrix,
        _camera.distortion
      ) == 0
    ) {
      return false;
    }

    if (
      !cv::aruco::estimatePoseCharucoBoard(
        charuco_corners,
        charuco_ids,
        board,
        _camera.matrix,
        _camera.distortion,
        _camera.rotation,
        _camera.translation
      )
    ) {
      return false;
    }

    cv::aruco::drawAxis(
      image_copy,
      _camera.matrix,
      _camera.distortion,
      _camera.rotation,
      _camera.translation,
      0.1f
    );
    cv::imshow(view_name, image_copy);

    return true;
  }

  void _process_frame_loop() {
    while (true) {
      std::unique_lock<std::mutex> lock{_mutex};
      _ready_flag.wait(lock, [this]() { return _ready.load(); });

      _process_frame();

      _ready = false;
      _ready_flag.notify_all();
    }
  }

  void _process_frame() {
    std::cout << _camera_id << ": process frame." << std::endl;

    // Capture frame from camera and undistort.
    cv::Mat image = _capture();
    cv::Mat undistorted_image;
    cv::fisheye::undistortImage(
      image,
      undistorted_image,
      _camera.matrix,
      _camera.distortion,
      _camera.undistorted_matrix
    );

    // Track skeleton in frame.


    // Reverse projection for each point in skeleton.
    // https://stackoverflow.com/questions/51272055/opencv-unproject-2d-points-to-3d-with-known-depth-z
    //
    // std::vector<cv::Point3d> result;
    // result.reserve(points.size());
    // for (size_t idx = 0; idx < points_undistorted.size(); ++idx) {
    //   const double z = Z.size() == 1 ? Z[0] : Z[idx];
    //   result.push_back(
    //       cv::Point3d((points_undistorted[idx].x - c_x) / f_x * z,
    //                   (points_undistorted[idx].y - c_y) / f_y * z, z));
    // }

    // std::vector<Skeleton3D> projected_skeletons;
    // std::size_t skel_count = static_cast<std::size_t>(skeletons->numSkeletons);
    // projected_skeletons.reserve(skel_count);
    // for (std::size_t i = 0; i < skel_count; ++i) {
    //   const CM_SKEL_KeypointsBuffer& cm_skeleton = skeletons->skeletons[i];
    //   Skeleton3D skeleton_3d{.skeleton_id = cm_skeleton.id};

    //   for (std::size_t point_idx = 0; point_idx < SKELETON_POINT_COUNT; ++point_idx) {
    //     const cv::Point2f point{
    //       cm_skeleton.keypoints_coord_x[point_idx],
    //       cm_skeleton.keypoints_coord_y[point_idx]
    //     };
    //     if (point == ABSENT_SKELETON_POINT) continue;

    //     skeleton_3d.points[point_idx] = Skeleton3D::Point{
    //       .point_id = static_cast<int>(point_idx),
    //       .confidence = cm_skeleton.confidences[point_idx],
    //       .coords{
    //         (point.x - c_x()) / f_x(),
    //         (point.y - c_y()) / f_y(),
    //         1
    //       }
    //     };

    //     std::cout
    //       << _camera_id << '.' << skeleton_3d.skeleton_id << '.' << point_idx
    //       << ": " << skeleton_3d.points[point_idx]->coords.x << ", "
    //       << skeleton_3d.points[point_idx]->coords.y << ", "
    //       << skeleton_3d.points[point_idx]->coords.z << std::endl;
    //   }

    //   projected_skeletons.push_back(std::move(skeleton_3d));
    // }

    // Move points from camera origin to scene origin.
  }

  void _track_skeletons(cv::Mat image) {
  }

  cv::Mat _capture() {
    cv::Mat view;
    _camera_input >> view;
    return view;
  }

  int _camera_id;
  CameraDetails _camera;
  cv::VideoCapture _camera_input;

  std::atomic_bool _ready;
  std::thread _thread;
  std::mutex _mutex;
  std::condition_variable _ready_flag;
};


int main() {
  FrameProcessor processor_0{0};

  processor_0.start();
  while(true) {
    std::cout << "x: loop" << std::endl;
    processor_0.wait();
    processor_0.tick();

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // TODO: calculate intersection in 3d of each skeleton point from cameras.
  }
}
