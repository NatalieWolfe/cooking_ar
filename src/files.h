#pragma once

#include <filesystem>

const std::filesystem::path& get_output_root_path();
const std::filesystem::path& get_recordings_directory_path();
const std::filesystem::path& get_calibration_directory_path();

std::filesystem::path get_recordings_path(int camera_id);
std::filesystem::path get_calibration_path(int camera_id);
