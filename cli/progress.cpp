#include "cli/progress.h"

#include <iostream>

namespace cli {

Progress::~Progress() {
  if (!_stream.str().empty()) print();
  if (!_last_message.empty()) std::cout << std::endl;
}

void Progress::print() {
  for (std::size_t i = 0; i < _last_message.size(); ++i) {
    std::cout << '\b';
  }
  _stream << ' ';
  _last_message = _stream.str();
  _stream.str("");
  std::cout << _last_message << std::flush;
}

}
