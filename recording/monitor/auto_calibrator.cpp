
#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <string>
#include <thread>
#include <vector>

#include "recording/monitor/charuco.h"

using namespace std::chrono_literals;
using namespace recording;

using std::chrono::duration_cast;

constexpr std::size_t POOL_SIZE = 24;

struct Frame {
  std::filesystem::path path;
  cv::Mat image;
  cv::Mat corners;
  cv::Mat corner_ids;
};

class FrameSelector {
 public:
  explicit FrameSelector(std::size_t data_point_count):
    _data_point_count{data_point_count}
  {
    _counters.reserve(data_point_count);
    _counters.push_back(0);
  }

  std::optional<std::vector<std::size_t>> operator++() {
    std::scoped_lock<std::mutex> lock{_counter_guard};
    if (!_increment()) {
      if (_counters.size() == _data_point_count) return std::nullopt;
      std::size_t new_size = _counters.size() + 1;
      _counters.clear();
      for (std::size_t i = 0; i < new_size; ++i) _counters.push_back(i + 1);
    }
    return _counters;
  }

 private:
  bool _increment() {
    for (int64_t i = _counters.size() - 1; i >= 0; --i) {
      if (_counters[i] < _data_point_count - (_counters.size() - i - 1)) {
        ++_counters[i];
        return true;
      }
    }
    return false;
  }

  std::size_t _data_point_count;
  std::vector<std::size_t> _counters;
  std::mutex _counter_guard;
};

template <typename Func>
void pool(std::size_t count, Func&& func) {
  std::vector<std::unique_ptr<std::thread>> threads;
  for (std::size_t i = 0; i < count; ++i) {
    threads.push_back(std::make_unique<std::thread>(std::forward<Func>(func)));
  }
  for (auto& thread : threads) thread->join();
}

std::vector<Frame> load_frames() {
  auto calibration_board = get_charuco_board();
  std::filesystem::path frame_dir = "res/frames";
  std::vector<std::filesystem::path> files;
  for (const auto& entry : std::filesystem::directory_iterator(frame_dir)) {
    files.push_back(entry.path());
  }

  std::cout << "Loading from " << frame_dir << std::endl;
  std::vector<Frame> frames{files.size(), Frame{}};
  std::atomic_size_t total = 0;
  pool(POOL_SIZE, [&]() {
    auto calibrator = std::make_unique<CharucoCalibrator>(calibration_board);
    std::size_t i;
    while ((i = total++), i < files.size()) {
      Frame& frame = frames[i];
      frame.path = files[i];
      frame.image = cv::imread(files[i].string());
      auto charuco = calibrator->extract_charuco(frame.image);
      if (charuco && charuco->first.rows > 6) {
        std::tie(frame.corners, frame.corner_ids) = std::move(*charuco);
      }
    }
  });

  std::vector<Frame> results;
  results.reserve(frames.size());
  std::copy_if(
    frames.begin(), frames.end(),
    std::back_inserter(results),
    [](const Frame& frame) { return !frame.corners.empty(); }
  );
  results.shrink_to_fit();

  std::cout
    << "Loaded " << results.size() << " boards from " << files.size()
    << " files." << std::endl;
  return results;
}

double calibrate(
  const std::vector<Frame>& frames,
  const std::vector<std::size_t>& selection
) {
  CharucoCalibrator calibrator{get_charuco_board()};
  for (std::size_t i : selection) {
    calibrator.add_corners(
      frames.at(i - 1).corners,
      frames.at(i - 1).corner_ids
    );
  }
  calibrator.set_latest_frame(
    frames[selection.front()].image,
    /*extract_board=*/false
  );
  double error_rate = calibrator.calibrate();
  return calibrator.calibrated() ? error_rate : 420.69;
}

int main() {
  std::vector<Frame> frames = load_frames();

  FrameSelector selector{frames.size()};
  std::atomic<double> best_error_rate = 420.69;
  std::atomic_size_t total_tested = 0;
  std::mutex frame_guard;
  std::vector<std::size_t> best_frames;

  std::thread observer{[&]() {
    std::size_t previous_total = 0;
    do {
      std::size_t total_copy = total_tested;
      std::size_t increase = total_copy - previous_total;
      previous_total = total_copy;
      std::cout
        << increase << " more tested, " << total_copy << " total." << std::endl;

      std::this_thread::sleep_for(5min);
    } while (total_tested != previous_total);
  }};

  pool(POOL_SIZE, [&]() {
    std::size_t tested = 0;
    std::string last_error;
    std::size_t last_error_count = 0;
    std::chrono::nanoseconds dur;
    std::optional<std::vector<std::size_t>> selection;

    while ((selection = ++selector), selection) {
      if (selection->size() < 8) continue;
      ++tested;
      ++total_tested;

      auto start = std::chrono::steady_clock::now();
      double error_rate = 420.69;
      try {
        error_rate = calibrate(frames, *selection);
      } catch (const cv::Exception& err) {
        if (last_error != err.what()) {
          std::cout << "W: " << err.what() << std::endl;
          last_error = err.what();
          last_error_count = 1;
        } else {
          ++last_error_count;
        }
        continue;
      }
      if (last_error_count > 1) {
        std::cout << "Error x " << last_error_count << std::endl;
        last_error_count = 0;
        last_error.clear();
      }
      dur += std::chrono::steady_clock::now() - start;
      if (error_rate >= best_error_rate) continue;

      std::scoped_lock<std::mutex> lock{frame_guard};
      if (error_rate >= best_error_rate) continue;

      best_error_rate = error_rate;
      best_frames = *selection;
      std::cout << "--------------------\n" << error_rate << std::endl;
      std::cout
        << tested << " of " << total_tested << " in "
        << duration_cast<std::chrono::seconds>(dur).count() << " seconds ("
        << duration_cast<std::chrono::milliseconds>(dur / tested).count()
        << " ms average)" << std::endl;
      tested = 0;
      dur = 0ns;
      for (std::size_t i : best_frames) {
        std::cout << frames[i - 1].path << std::endl;
      }
    }
  });

  return 0;
}
