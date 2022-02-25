#include "extraction/pose_extractor.h"

#include <experimental/source_location>
#include <filesystem>
#include <memory>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <span>
#include <string_view>
#include <vector>

#include "extraction/pose3d.h"
#include "lw/err/canonical.h"
#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/port/statusor.h"
#include "mediapipe/framework/port/status_macros.h"

namespace extraction {
namespace {

using ::mediapipe::CalculatorGraph;
using ::mediapipe::CalculatorGraphConfig;
using ::mediapipe::file::GetContents;
using ::mediapipe::LandmarkList;
using ::mediapipe::NormalizedLandmarkList;
using ::mediapipe::OutputStreamPoller;
using ::std::experimental::source_location;
using ::std::filesystem::path;

constexpr std::string_view INPUT_STREAM = "input_video";
// 3D Pose landmarks. (LandmarkList)
constexpr std::string_view POSE_WORLD_LANDMARKS = "pose_world_landmarks";
// 3D-ish Face landmarks. (NormalizedLandmarkList)
constexpr std::string_view FACE_LANDMARKS = "face_landmarks";
// Left paw landmarks. (NormalizedLandmarkList)
constexpr std::string_view LEFT_PAW_LANDMARKS = "left_paw_landmarks";
// Right paw landmarks. (NormalizedLandmarkList)
constexpr std::string_view RIGHT_PAW_LANDMARKS = "right_paw_landmarks";

constexpr std::string_view GRAPH_PROTOTEXT_PATH =
  "extraction/mediapipe_graph.pbtxt";

void assert_ok(
  const absl::Status& status,
  const source_location& loc = source_location::current()
) {
  if (!status.ok()) {
    throw lw::Internal(loc) << status.message();
  }
}

template <typename T>
T assert_ok(
  absl::StatusOr<T> status_or,
  const source_location& loc = source_location::current()
) {
  assert_ok(status_or.status(), loc);
  return std::move(status_or).value();
}

std::unique_ptr<CalculatorGraph> load_graph() {
  std::string graph_config_text;
  assert_ok(GetContents(GRAPH_PROTOTEXT_PATH, &graph_config_text));
  auto graph_config =
    mediapipe::ParseTextProtoOrDie<CalculatorGraphConfig>(graph_config_text);
  auto graph = std::make_unique<CalculatorGraph>();
  assert_ok(graph->Initialize(graph_config));
  return graph;
}

OutputStreamPoller add_poller(CalculatorGraph& graph, std::string_view stream) {
  return assert_ok(graph.AddOutputStreamPoller(std::string{stream}));
}

std::unique_ptr<mediapipe::ImageFrame> load_frame(const path& frame_path) {
  cv::Mat frame_raw = cv::imread(frame_path.string());
  cv::Mat cam_frame;
  cv::cvtColor(frame_raw, cam_frame, cv::COLOR_BGR2RGB);

  // Load the frame into a Mediapipe frame.
  auto input_frame = std::make_unique<mediapipe::ImageFrame>(
    mediapipe::ImageFormat::SRGB,
    cam_frame.cols,
    cam_frame.rows,
    mediapipe::ImageFrame::kDefaultAlignmentBoundary
  );
  cv::Mat input_frame_mat = mediapipe::formats::MatView(input_frame.get());
  cam_frame.copyTo(input_frame_mat);
  return input_frame;
}

template <typename LandmarkListT>
std::vector<Pose3d::Point> to_points(const LandmarkListT& list) {
  std::vector<Pose3d::Point> points;
  for (int point_idx = 0; point_idx < list.landmark().size(); ++point_idx) {
    const auto& landmark = list.landmark(point_idx);
    points.push_back({
      .point_id = point_idx,
      .x = landmark.x(),
      .y = landmark.y(),
      .z = landmark.z(),
      .confidence = landmark.visibility() * landmark.presence()
    });
  }
  return points;
}

template <typename LandmarkListT>
std::vector<Pose3d::Point> poll_landmarks(OutputStreamPoller& poller) {
  mediapipe::Packet packet;
  if (!poller.Next(&packet)) {
    throw lw::Unimplemented()
      << "Handling failed OutputStreamPoller::Next() is not implemented yet.";
  }

  return to_points(packet.Get<LandmarkListT>());
}

}

PoseExtractor::PoseExtractor():
  _graph{load_graph()},
  _pose_world_landmarks{add_poller(*_graph, POSE_WORLD_LANDMARKS)},
  _face_landmarks{add_poller(*_graph, FACE_LANDMARKS)},
  _left_paw_landmarks{add_poller(*_graph, LEFT_PAW_LANDMARKS)},
  _right_paw_landmarks{add_poller(*_graph, RIGHT_PAW_LANDMARKS)}
{
  assert_ok(_graph->StartRun({}));
}

PoseExtractor::~PoseExtractor() {
  if (_graph) {
    assert_ok(_graph->CloseInputStream(std::string{INPUT_STREAM}));
    assert_ok(_graph->WaitUntilDone());
  }
}

std::vector<Pose3d> PoseExtractor::get(const path& frame_path) {
  std::unique_ptr<mediapipe::ImageFrame> image = load_frame(frame_path);

  // Send image to the graph.
  std::size_t frame_timestamp_us =
    (double)cv::getTickCount() / (double)cv::getTickFrequency() * 1e6;
  assert_ok(
    _graph->AddPacketToInputStream(
      std::string{INPUT_STREAM},
      mediapipe::Adopt(image.release())
        .At(mediapipe::Timestamp(frame_timestamp_us))
    )
  );

  return _data_to_poses();
}

std::vector<Pose3d> PoseExtractor::_data_to_poses() {
  // TODO(alaina): Implement handling multiple people captured in the frame.
  std::vector<Pose3d> poses;
  poses.reserve(1);
  poses.push_back(Pose3d{
    .person_id = 0,
    .body = poll_landmarks<LandmarkList>(_pose_world_landmarks),
    .face = poll_landmarks<NormalizedLandmarkList>(_face_landmarks),
    .left_paw = poll_landmarks<NormalizedLandmarkList>(_left_paw_landmarks),
    .right_paw = poll_landmarks<NormalizedLandmarkList>(_right_paw_landmarks)
  });
  return poses;
}

}
