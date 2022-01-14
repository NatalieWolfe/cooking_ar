#include "recording/oakd_camera.h"

#include "gtest/gtest.h"

namespace recording {
namespace {

TEST(OakDCamera, GetFrames) {
  OakDCamera cam = OakDCamera::make();
  OakDFrames frames = cam.get();

  EXPECT_NE(frames.right, nullptr);
  EXPECT_NE(frames.left, nullptr);
}

}
}
