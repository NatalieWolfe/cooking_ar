#pragma once

#include <chrono>

namespace recording {

enum class Key: int {
  NONE = -1,

  ENTER = 13,
  ESC = 27,
  SPACE = 32,
  ONE = 49,
  TWO = 50,
  THREE = 51,
  FOUR = 52,
  LEFT = 81,
  DOWN = 82,
  RIGHT = 83,
  UP = 84,
  E = 101,
  F = 102,
  M = 109,
  Q = 113,
  R = 114,
  W = 119,
  X = 120,
  Z = 122
};

Key wait_key(std::chrono::milliseconds duration);

}
