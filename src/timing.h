#pragma once

#include <chrono>

using steady_clock = std::chrono::steady_clock;

double to_fps(std::size_t frame_count, const steady_clock::duration& duration);
