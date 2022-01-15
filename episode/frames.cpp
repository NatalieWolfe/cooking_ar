#include "episode/frames.h"

#include <filesystem>
#include <sstream>
#include <string>
#include <string_view>

namespace episode {
namespace {

using ::std::filesystem::directory_entry;
using ::std::filesystem::directory_iterator;

constexpr std::string_view EXTENSION = ".png";

std::string frame_file(std::size_t id) {
  std::stringstream name;
  name << std::setw(8) << std::setfill('0') << std::right << id << EXTENSION;
  return name.str();
}

}

FrameRange::Iterator::Iterator(const FrameRange& range, std::size_t counter):
  _range{range},
  _counter{counter},
  _path{range.path() / frame_file(counter)}
{}

FrameRange::Iterator& FrameRange::Iterator::operator++() {
  _path = _range.path() / frame_file(++_counter);
  return *this;
}

bool FrameRange::Iterator::operator==(const Iterator& other) const {
  return _counter == other._counter && _range.path() == other._range.path();
}

std::size_t FrameRange::size() const {
  std::size_t counter = 0;
  for (const directory_entry& entry : directory_iterator{_root}) {
    if (entry.path().extension() == EXTENSION) ++counter;
  }
  return counter;
}

}
