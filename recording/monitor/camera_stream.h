#pragma once

#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>

namespace recording {

class CameraStream {
public:
  // Connects to the given host and starts the stream.
  //
  // @throws std::runtime_error
  //  If any error occurs opening the socket or starting the stream.
  //
  // @param stream_host
  //  The hostname of the camera's streaming server.
  static CameraStream connect(std::string_view stream_host);

  CameraStream(const CameraStream&) = delete;
  CameraStream& operator=(const CameraStream&) = delete;

  CameraStream(CameraStream&& other);
  CameraStream& operator=(CameraStream&& other);

  // Closes the stream socket.
  ~CameraStream();

  // Reads a single frame from the camera stream and writes it to the given
  // stream.
  //
  // @throws std::runtime_error
  //  If the read fails to produce a frame.
  //
  // @param out
  //  The stream to save the read frame into.
  void read_frame(std::ostream& out);

private:
  explicit CameraStream(int socket, std::string frame_boundary):
    _socket{socket},
    _frame_boundary{std::move(frame_boundary)}
  {}

  int _socket = 0;
  std::string _frame_boundary;
};

}
