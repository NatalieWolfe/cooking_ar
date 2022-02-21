#include <iostream>
#include <memory>
#include <string>
#include <string_view>

#include "mediapipe/framework/calculator_framework.h"
#include "mediapipe/framework/formats/image_frame.h"
#include "mediapipe/framework/formats/image_frame_opencv.h"
#include "mediapipe/framework/formats/landmark.pb.h"
#include "mediapipe/framework/port/file_helpers.h"
#include "mediapipe/framework/port/opencv_highgui_inc.h"
#include "mediapipe/framework/port/opencv_video_inc.h"
#include "mediapipe/framework/port/parse_text_proto.h"
#include "mediapipe/framework/port/status.h"
#include "mediapipe/framework/port/statusor.h"
#include "mediapipe/framework/port/status_macros.h"

constexpr std::string_view INPUT_STREAM = "input_video";
constexpr std::string_view OUTPUT_STREAM = "output_video";
constexpr std::string_view OUTPUT_LANDMARKS = "pose_world_landmarks";

absl::StatusOr<std::unique_ptr<mediapipe::CalculatorGraph>> load_graph() {
  std::string graph_config_text;
  MP_RETURN_IF_ERROR(
    mediapipe::file::GetContents(
      "extraction/mediapipe_graph.pbtxt",
      &graph_config_text
    )
  );
  auto graph_config =
    mediapipe::ParseTextProtoOrDie<mediapipe::CalculatorGraphConfig>(
      graph_config_text
    );
  auto graph = std::make_unique<mediapipe::CalculatorGraph>();
  MP_RETURN_IF_ERROR(graph->Initialize(graph_config));
  return graph;
}

std::unique_ptr<mediapipe::ImageFrame> capture_frame(cv::VideoCapture& capture) {
  // Capture frame from disk and convert the color space.
  cv::Mat frame_raw;
  capture >> frame_raw;
  if (frame_raw.empty()) return nullptr; // End of video.
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

absl::Status run_graph() {
  ASSIGN_OR_RETURN(
    std::unique_ptr<mediapipe::CalculatorGraph> graph,
    load_graph()
  );

  cv::VideoCapture capture;
  capture.open(0);
  RET_CHECK(capture.isOpened());
  capture.set(cv::CAP_PROP_FPS, 30);

  ASSIGN_OR_RETURN(
    mediapipe::OutputStreamPoller out_frame_poller,
    graph->AddOutputStreamPoller(std::string{OUTPUT_STREAM})
  );
  ASSIGN_OR_RETURN(
    mediapipe::OutputStreamPoller pose_poller,
    graph->AddOutputStreamPoller(std::string{OUTPUT_LANDMARKS})
  );
  MP_RETURN_IF_ERROR(graph->StartRun({}));

  std::size_t pose_counter = 0;
  while (cv::waitKey(/*delay=*/5) < 0) {
    auto input_frame = capture_frame(capture);
    if (!input_frame) break; // End of video.

    // Send image to the graph.
    std::size_t frame_timestamp_us =
      (double)cv::getTickCount() / (double)cv::getTickFrequency() * 1e6;
    MP_RETURN_IF_ERROR(
      graph->AddPacketToInputStream(
        std::string{INPUT_STREAM},
        mediapipe::Adopt(input_frame.release())
          .At(mediapipe::Timestamp(frame_timestamp_us))
      )
    );

    // Read the result back.
    mediapipe::Packet frame_packet;
    if (!out_frame_poller.Next(&frame_packet)) break; // ???
    auto& out_frame = frame_packet.Get<mediapipe::ImageFrame>();

    // Convert the frame to OpenCV format and display it.
    cv::Mat out_frame_mat = mediapipe::formats::MatView(&out_frame);
    cv::cvtColor(out_frame_mat, out_frame_mat, cv::COLOR_RGB2BGR);
    cv::imshow("Window", out_frame_mat);

    if (pose_poller.QueueSize() == 0) continue;
    ++pose_counter;
    mediapipe::Packet pose_packet;
    if (!pose_poller.Next(&pose_packet)) break; // Pose acquisition failed?
    if (pose_counter % 30) continue; // Only print pose info a 1fps.

    // pose_landmarks -> NormalizedLandmarkList
    // pose_world_landmarks -> LandmarkList
    auto& pose = pose_packet.Get<mediapipe::LandmarkList>();
    int id = 0;
    for (const auto& landmark : pose.landmark()) {
      std::cout
        << (id++) << ": " << landmark.x() << "," << landmark.y() << ","
        << landmark.z() << ": " << landmark.visibility() << "; "
        << landmark.presence() << std::endl;
    }
    std::cout
      << std::endl
      << "---------------------------------------------------------------------"
      << std::endl << std::endl;
  }

  MP_RETURN_IF_ERROR(graph->CloseInputStream(std::string{INPUT_STREAM}));
  return graph->WaitUntilDone();
}

int main(int argc, char** argv) {
  absl::Status status = run_graph();
  std::cout << "Final status: " << status << std::endl;
  return 0;
}
