#pragma once

#include <memory>

#include "extraction/pose3d.h"
#include "mediapipe/framework/calculator_framework.h"

namespace extraction {

class PoseExtractor {
public:
  PoseExtractor();
  ~PoseExtractor();

  PoseExtractor(const PoseExtractor&) = delete;
  PoseExtractor& operator=(const PoseExtractor&) = delete;

  /**
   * @brief Extracts the pose information from the image stored at the given
   * file path.
   *
   * @param frame_path The file path to the image.
   */
  std::vector<Pose3d> get(const std::filesystem::path& frame_path);
private:
  std::vector<Pose3d> _data_to_poses();

  std::unique_ptr<mediapipe::CalculatorGraph> _graph;
  mediapipe::OutputStreamPoller _pose_world_landmarks;
  mediapipe::OutputStreamPoller _face_landmarks;
  mediapipe::OutputStreamPoller _left_paw_landmarks;
  mediapipe::OutputStreamPoller _right_paw_landmarks;
};

}
