#include "episode/project.h"

#include <filesystem>
#include <string_view>

#include "gtest/gtest.h"

namespace episode {
namespace {

using std::filesystem::exists;
using std::filesystem::path;
using std::filesystem::remove_all;

const path CAMERA_DIR = "cameras";
const path PROJECTS_DIR = "/tmp/testing/ar/episode/project";
constexpr std::string_view TEST_CAM = "test_cam_1";

void clean_slate() {
  remove_all(PROJECTS_DIR);
}

std::string_view test_name() {
  return testing::UnitTest::GetInstance()->current_test_info()->name();
}

TEST(Project, Open) {
  clean_slate();
  Project p = Project::open(PROJECTS_DIR / test_name());
  ASSERT_TRUE(exists(PROJECTS_DIR / test_name()));
  EXPECT_TRUE(exists(PROJECTS_DIR / test_name() / CAMERA_DIR));

  EXPECT_EQ(p.directory(), PROJECTS_DIR / test_name());
  EXPECT_EQ(p.name(), test_name());
}

TEST(Project, Destroy) {
  Project p = Project::open(PROJECTS_DIR / test_name());
  ASSERT_TRUE(exists(PROJECTS_DIR / test_name()));

  Project::destroy(p.directory());
  EXPECT_FALSE(exists(PROJECTS_DIR / test_name()));
}

TEST(Project, AddCamera) {
  clean_slate();
  Project p = Project::open(PROJECTS_DIR / test_name());
  const CameraDirectory& cam = p.add_camera(TEST_CAM);

  EXPECT_EQ(cam.name, TEST_CAM);
  EXPECT_EQ(cam.path.filename(), TEST_CAM);
  EXPECT_EQ(cam.path.parent_path().filename(), CAMERA_DIR);
  EXPECT_TRUE(p.has_camera(TEST_CAM));

  ASSERT_TRUE(exists(cam.path));
  EXPECT_TRUE(exists(cam.left_recording));
  EXPECT_TRUE(exists(cam.right_recording));
  EXPECT_FALSE(exists(cam.calibration_file));
}

TEST(Project, HasCamera) {
  clean_slate();
  Project p = Project::open(PROJECTS_DIR / test_name());
  EXPECT_FALSE(p.has_camera(TEST_CAM));

  p.add_camera(TEST_CAM);
  EXPECT_TRUE(p.has_camera(TEST_CAM));
}

TEST(Project, GetCamera) {
  clean_slate();
  Project p = Project::open(PROJECTS_DIR / test_name());
  const CameraDirectory& cam = p.camera(TEST_CAM);

  EXPECT_EQ(cam.name, TEST_CAM);
  EXPECT_EQ(cam.path.filename(), TEST_CAM);
  EXPECT_EQ(cam.path.parent_path().filename(), CAMERA_DIR);

  // Camera information was fetched, not added.
  EXPECT_FALSE(exists(cam.path));
  EXPECT_FALSE(p.has_camera(TEST_CAM));
}

}
}
