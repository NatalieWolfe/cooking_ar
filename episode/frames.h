#pragma once

#include <filesystem>

namespace episode {

class FrameRange {
public:
  class Iterator {
  public:
    const std::filesystem::path& operator*() const { return _path; }
    const std::filesystem::path* operator->() const { return &_path; }

    Iterator& operator++();

    bool operator==(const Iterator& other) const;
    bool operator!=(const Iterator& other) const { return !(*this == other); }

  private:
    friend class FrameRange;
    explicit Iterator(const FrameRange& range, std::size_t counter);

    const FrameRange& _range;
    std::size_t _counter;
    std::filesystem::path _path;
  };

  explicit FrameRange(std::filesystem::path dir): _root{std::move(dir)} {}

  Iterator begin() const { return Iterator{*this, 1}; }
  Iterator end() const { return Iterator{*this, size() + 1}; }

  /**
   * @brief Iterates through the frame directory and counts how many image files
   * are present.
   *
   * @return The number of files in the directory ending with `.png`.
   */
  std::size_t size() const;

  const std::filesystem::path& path() const { return _root; }

private:
  std::filesystem::path _root;
};

}
