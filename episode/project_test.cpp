#include "episode/project.h"

#include <filesystem>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lw/err/canonical.h"

namespace episode {
namespace {

using ::std::filesystem::create_directories;
using ::std::filesystem::exists;
using ::std::filesystem::path;
using ::std::filesystem::remove_all;
using ::testing::MatchesRegex;

const path CAMERA_DIR = "cameras";
const path SESSION_DIR = "sessions";
const path PROJECTS_DIR = "/tmp/testing/ar/episode/project";
constexpr std::string_view TEST_CAM = "test_cam_1";

void clean_slate() {
  remove_all(PROJECTS_DIR);
}

std::string_view test_name() {
  return testing::UnitTest::GetInstance()->current_test_info()->name();
}

TEST(Project, OpenNotFound) {
  clean_slate();
  EXPECT_THROW(Project::open(PROJECTS_DIR / test_name()), lw::NotFound);
}

TEST(Project, Open) {
  const path project_path = PROJECTS_DIR / test_name();
  create_directories(project_path);
  Project p = Project::open(project_path);
  EXPECT_EQ(p.directory(), project_path);
  EXPECT_FALSE(p.has_session()) << "Should not have active session.";
}

TEST(Project, NewSession) {
  clean_slate();
  Project p = Project::new_session(PROJECTS_DIR / test_name());
  ASSERT_TRUE(exists(PROJECTS_DIR / test_name()));
  EXPECT_TRUE(exists(p.session_directory() / CAMERA_DIR));

  EXPECT_EQ(p.directory(), PROJECTS_DIR / test_name());
  EXPECT_THAT(
    p.session_directory(),
    MatchesRegex(
      (PROJECTS_DIR / test_name() / SESSION_DIR).string() +
      R"re(/[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2})re"
    )
  );
  EXPECT_EQ(p.name(), test_name());
  EXPECT_TRUE(p.has_session()) << "Should have active session.";
}

TEST(Project, Destroy) {
  Project p = Project::new_session(PROJECTS_DIR / test_name());
  ASSERT_TRUE(exists(PROJECTS_DIR / test_name()));

  Project::destroy(p.directory());
  EXPECT_FALSE(exists(PROJECTS_DIR / test_name()));
}

TEST(Project, Sessions) {
  Project p = Project::new_session(PROJECTS_DIR / test_name());
  std::vector<std::string> session_ids = p.sessions();
  EXPECT_FALSE(session_ids.empty());
  for (const std::string& session_id : session_ids) {
    EXPECT_TRUE(p.has_session(session_id));
  }
}

TEST(Project, AddCamera) {
  clean_slate();
  Project p = Project::new_session(PROJECTS_DIR / test_name());
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

TEST(Project, AddCameraOutsideSession) {
  const path project_path = PROJECTS_DIR / test_name();
  create_directories(project_path);
  Project p = Project::open(project_path);
  EXPECT_THROW(p.add_camera(TEST_CAM), lw::FailedPrecondition);
}

TEST(Project, HasCamera) {
  clean_slate();
  Project p = Project::new_session(PROJECTS_DIR / test_name());
  EXPECT_FALSE(p.has_camera(TEST_CAM));

  p.add_camera(TEST_CAM);
  EXPECT_TRUE(p.has_camera(TEST_CAM));
}

TEST(Project, GetCamera) {
  clean_slate();
  Project p = Project::new_session(PROJECTS_DIR / test_name());
  const CameraDirectory& added_cam = p.add_camera(TEST_CAM);
  const CameraDirectory& cam = p.camera(TEST_CAM);

  EXPECT_EQ(cam.name, TEST_CAM);
  EXPECT_EQ(cam.path.filename(), TEST_CAM);
  EXPECT_EQ(cam.path.parent_path().filename(), CAMERA_DIR);

  EXPECT_EQ(cam.name, added_cam.name);
  EXPECT_EQ(cam.path, added_cam.path);
  EXPECT_EQ(cam.left_recording, added_cam.left_recording);
  EXPECT_EQ(cam.right_recording, added_cam.right_recording);
  EXPECT_EQ(cam.calibration_file, added_cam.calibration_file);
}

}
}
