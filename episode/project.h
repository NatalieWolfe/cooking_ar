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

/**
 * @brief Manages filesystem directories for a project.
 *
 * A project directory tree looks like:
 * - /path/to/project
 *   - <project_name>/
 *     - sessions/
 *       - <session_id>/
 *         - cameras/
 *           - <camera_name>/
 *             - calibration.json
 *             - recordings/
 *               - <subcamera_name>/
 *                 - <frame_id>.png
 */
class Project {
public:
  /**
   * Opens the project at the given path. Any missing directories are created.
   */
  static Project open(std::filesystem::path dir);
  static void destroy(const std::filesystem::path& dir);

  Project(const Project&) = delete;
  Project& operator=(const Project&) = delete;

  Project(Project&&) = default;
  Project& operator=(Project&&) = default;

  const std::filesystem::path& directory() const { return _root; }
  std::string_view name() const { return _name; }

  std::filesystem::path session_directory() const;

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
  explicit Project(std::filesystem::path dir, std::string session_id);

  std::filesystem::path _root;
  std::string _name;
  std::string _session_id;
};

}
