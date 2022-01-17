#pragma once

#include <sstream>
#include <string>

namespace cli {

class Progress {
public:
  Progress() = default;
  ~Progress();

  std::stringstream& stream() { return _stream; }
  void print();

private:
  std::stringstream _stream;
  std::string _last_message;
};

}
