#pragma once

#include <chrono>
#include <string>

using steady_clock = std::chrono::steady_clock;

double to_fps(std::size_t frame_count, const steady_clock::duration& duration);
std::string to_hms(steady_clock::duration duration);
