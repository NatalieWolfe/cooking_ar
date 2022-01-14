#pragma once

#include <memory>

#include <depthai/depthai.hpp>

namespace recording {

struct OakDFrames {
  std::shared_ptr<dai::ImgFrame> right;
  std::shared_ptr<dai::ImgFrame> left;
};

class OakDCamera {
public:
  /**
   * @brief Initializes a new OakD camera interface.
   *
   * @todo Take in a device path to select which camera to initialize.
   */
  static OakDCamera make();

  OakDCamera(const OakDCamera&) = delete;
  OakDCamera& operator=(const OakDCamera&) = delete;
  OakDCamera(OakDCamera&&) = default;
  OakDCamera& operator=(OakDCamera&&) = default;

  /**
   * @brief Blocks until the next frame is available from each OakD camera on
   * the linked device.
   */
  OakDFrames get();

private:
  explicit OakDCamera(
    std::unique_ptr<dai::Pipeline> pipeline,
    std::unique_ptr<dai::Device> device,
    std::shared_ptr<dai::DataOutputQueue> right_queue,
    std::shared_ptr<dai::DataOutputQueue> left_queue
  );

  std::unique_ptr<dai::Pipeline> _pipeline;
  std::unique_ptr<dai::Device> _device;
  std::shared_ptr<dai::DataOutputQueue> _right_queue;
  std::shared_ptr<dai::DataOutputQueue> _left_queue;
};

}
