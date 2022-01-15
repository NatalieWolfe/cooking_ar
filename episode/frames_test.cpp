#include "episode/frames.h"

#include <filesystem>
#include <string>

#include "gtest/gtest.h"

namespace episode {
namespace {

using ::std::filesystem::path;

const path FRAMES_DIR = "episode/testing/fake_frames";

TEST(FrameRange, Path) {
  FrameRange frames{FRAMES_DIR};
  EXPECT_EQ(frames.path(), FRAMES_DIR);
}

TEST(FrameRange, Size) {
  FrameRange frames{FRAMES_DIR};
  EXPECT_EQ(frames.size(), 4);
}

TEST(FrameRange, Begin) {
  FrameRange frames{FRAMES_DIR};
  FrameRange::Iterator itr = frames.begin();
  EXPECT_EQ(*itr, FRAMES_DIR / "00000001.png");
}

TEST(FrameRange, End) {
  FrameRange frames{FRAMES_DIR};
  FrameRange::Iterator itr = frames.end();
  EXPECT_EQ(*itr, FRAMES_DIR / "00000005.png");
}

TEST(FrameRange, RangeBasedForLoop) {
  FrameRange frames{FRAMES_DIR};
  std::size_t counter = 0;
  for (const path& frame : frames) {
    EXPECT_EQ(
      frame,
      FRAMES_DIR / ("0000000" + std::to_string(++counter) + ".png")
    );
  }
  EXPECT_EQ(counter, 4);
}

}
}
