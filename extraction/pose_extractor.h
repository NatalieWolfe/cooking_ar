#pragma once

#include <openpose/headers.hpp>

#include "extraction/pose2d.h"

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
  std::vector<Pose2d> get(const std::filesystem::path& frame_path);
private:
  op::Wrapper _openpose;
};

}
