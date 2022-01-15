#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

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
   * @brief Opens an existing project at the given path.
   *
   * The project will be opened without an active session.
   */
  static Project open(std::filesystem::path dir);

  /**
   * @brief Opens the project at the given path with a new session.
   *
   * The project does not need to exist yet, all required directories are
   * created if missing.
   *
   * @note Session ids are based off the time when they are started. New
   * sessions for the same project cannot be started in rapid succession. This
   * is intentional.
   */
  static Project new_session(std::filesystem::path dir);

  static void destroy(const std::filesystem::path& dir);

  Project(const Project&) = delete;
  Project& operator=(const Project&) = delete;

  Project(Project&&) = default;
  Project& operator=(Project&&) = default;

  const std::filesystem::path& directory() const { return _root; }
  std::string_view name() const { return _name; }

  /**
   * @brief Returns the path to the identified session directory.
   *
   * @param session The session to fetch. Defaults to the current session.
   */
  std::filesystem::path session_directory(std::string_view session) const;
  std::filesystem::path session_directory() const;

  bool has_session(std::string_view session) const;
  bool has_session() const { return !_session_id.empty(); }

  /**
   * @brief Returns the list of sessions this project has had.
   *
   * The sessions are sorted by name.
   */
  std::vector<std::string> sessions() const;

  /**
   * @brief Creates the directory structure for a new camera in the project.
   *
   * The camera is added to the current session.
   */
  CameraDirectory add_camera(std::string_view name);

  /**
   * @brief Returns true if there is a directory in the project for the given
   * camera.
   *
   * @param name The name of the camera to look for.
   * @param session The session to look in. Defaults to the current session.
   */
  bool has_camera(std::string_view name, std::string_view session) const;
  bool has_camera(std::string_view name) const;

  /**
   * @brief Fetch the directory information for the given camera.
   *
   * @param name The name of the camera to fetch.
   * @param session The session to fetch from. Defaults to the current session.
   */
  CameraDirectory camera(std::string_view name) const;
  CameraDirectory camera(std::string_view name, std::string_view session) const;

private:
  explicit Project(std::filesystem::path dir, std::string session_id);

  std::filesystem::path _root;
  std::string _name;
  std::string _session_id;
};

}
