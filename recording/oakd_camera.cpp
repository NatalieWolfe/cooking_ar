#include "recording/oakd_camera.h"

#include <filesystem>
#include <memory>
#include <string_view>

#include <depthai/depthai.hpp>

namespace recording {
namespace {

constexpr std::size_t FRAME_BUFFER_LIMIT = 100;
constexpr float FRAMES_PER_SECOND = 60.0f;

void link_mono_output(
  dai::Pipeline& pipeline,
  std::string_view name,
  dai::CameraBoardSocket socket
) {
  auto cam = pipeline.create<dai::node::MonoCamera>();
  auto out = pipeline.create<dai::node::XLinkOut>();
  out->setStreamName(std::string{name});
  cam->setBoardSocket(socket);
  cam->setResolution(dai::MonoCameraProperties::SensorResolution::THE_480_P);
  cam->setFps(FRAMES_PER_SECOND);
  cam->out.link(out->input);
}

}

OakDCamera OakDCamera::make() {
  auto pipeline = std::make_unique<dai::Pipeline>();
  link_mono_output(*pipeline, "right", dai::CameraBoardSocket::RIGHT);
  link_mono_output(*pipeline, "left", dai::CameraBoardSocket::LEFT);

  auto device = std::make_unique<dai::Device>(*pipeline);
  auto right_queue = device->getOutputQueue("right", FRAME_BUFFER_LIMIT, true);
  auto left_queue = device->getOutputQueue("left", FRAME_BUFFER_LIMIT, true);

  return OakDCamera{
    std::move(pipeline),
    std::move(device),
    right_queue,
    left_queue
  };
}

OakDCamera::OakDCamera(
  std::unique_ptr<dai::Pipeline> pipeline,
  std::unique_ptr<dai::Device> device,
  std::shared_ptr<dai::DataOutputQueue> right_queue,
  std::shared_ptr<dai::DataOutputQueue> left_queue
):
  _pipeline{std::move(pipeline)},
  _device{std::move(device)},
  _right_queue{std::move(right_queue)},
  _left_queue{std::move(left_queue)}
{}

OakDFrames OakDCamera::get() {
  return {
    .right = _right_queue->get<dai::ImgFrame>(),
    .left = _left_queue->get<dai::ImgFrame>()
  };
}

void OakDCamera::save_calibration(const std::filesystem::path& path) const {
  _device->readCalibration().eepromToJsonFile(path.string());
}

}
