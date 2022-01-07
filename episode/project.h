#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace episode {

struct CameraDirectory {
  std::string name;
  std::filesystem::path path;
  std::filesystem::path left_recording;
  std::filesystem::path right_recording;
  std::filesystem::path calibration_file;
};

class Project {
public:
  /**
   * Opens the project at the given path. Any missing directories are created.
   */
  static Project open(std::filesystem::path dir);
  static void destroy(const std::filesystem::path& dir);

  const std::filesystem::path& directory() const { return _root; }
  std::string_view name() const { return _name; }

  /**
   * Creates the directory structure for a new camera in the project.
   */
  CameraDirectory add_camera(std::string_view name);

  /**
   * Returns true if there is a directory in the project for the given camera.
   */
  bool has_camera(std::string_view name) const;

  /**
   * Fetch the directory information for the given camera.
   */
  CameraDirectory camera(std::string_view name) const;

private:
  explicit Project(std::filesystem::path dir);

  std::filesystem::path _root;
  std::string _name;
};

}
