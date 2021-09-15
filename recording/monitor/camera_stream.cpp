#include "recording/monitor/camera_stream.h"

#include <cstdint>
#include <netdb.h>
#include <ostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

namespace recording {
namespace {

constexpr std::string_view SUCCESS_LINE = "HTTP/1.0 200 OK\r\n";

int open_socket(std::string_view stream_host) {
  ::addrinfo* addresses = nullptr;
  ::addrinfo hints{.ai_family = AF_UNSPEC, .ai_socktype = SOCK_STREAM};

  std::string host{stream_host};
  int err = ::getaddrinfo(host.c_str(), "5000", &hints, &addresses);
  if (err) throw std::runtime_error{"Failed to get address info."};

  ::addrinfo* mvr = addresses;
  for (; mvr != nullptr; mvr = mvr->ai_next) {
    int sock = ::socket(mvr->ai_family, mvr->ai_socktype, mvr->ai_protocol);
    if (sock <= 0) continue;

    if (::connect(sock, mvr->ai_addr, mvr->ai_addrlen) != 0) {
      ::close(sock);
      continue;
    }

    ::freeaddrinfo(addresses);
    return sock;
  }

  ::freeaddrinfo(addresses);
  throw std::runtime_error{"Failed to connect to camera host."};
}

std::string start_stream(int socket) {
  // Send a basic HTTP GET request.
  constexpr std::string_view request = "GET /stream.mjpg HTTP/1.0\r\n\r\n";
  int bytes_sent = ::send(socket, request.data(), request.size(), /*flags=*/0);
  if (bytes_sent != static_cast<int>(request.size())) {
    ::close(socket);
    throw std::runtime_error{"Failed to send stream request!"};
  }

  // Read the status line.
  char status_buffer[SUCCESS_LINE.size()] = {0};
  int bytes_received =
    ::recv(socket, (void*)status_buffer, sizeof(status_buffer), /*flags=*/0);
  std::string_view line_view{
    status_buffer,
    static_cast<std::size_t>(bytes_received)
  };
  if (bytes_received <=0 || line_view != SUCCESS_LINE) {
    ::close(socket);
    throw std::runtime_error{"Stream returned non-200 response."};
  }

  // Read all the headers.
  std::string line;
  std::string boundary;
  char c;
  while ((bytes_received = ::recv(socket, &c, 1, /*flags=*/0)) == 1) {
    line += c;
    if (!line.ends_with("\r\n")) continue;  // Line not complete yet.
    if (line.size() == 2) break;            // Headers done.
    if (!boundary.empty() || !line.starts_with("Content-Type")) {
      // Boundary already read or not the needed header.
      line.clear();
      continue;
    }

    // Parse the frame boundary out of the header.
    constexpr std::string_view boundary_prefix = "; boundary=";
    std::size_t prefix_pos = line.rfind(boundary_prefix);
    if (prefix_pos == std::string::npos) {
      ::close(socket);
      throw std::runtime_error{"No frame boundary found in Content-Type."};
    }
    std::size_t boundary_start = prefix_pos + boundary_prefix.size();
    boundary = line.substr(boundary_start, line.size() - boundary_start - 2);
  }

  if (boundary.empty() || bytes_received <= 0) {
    ::close(socket);
    throw std::runtime_error{"Failed to find frame boundary in headers."};
  }
  return "--" + boundary + "\r\n";
}

}

CameraStream CameraStream::connect(std::string_view stream_host) {
  int socket = open_socket(stream_host);
  std::string frame_boundary = start_stream(socket);
  return CameraStream{socket, std::move(frame_boundary)};
}

CameraStream::CameraStream(CameraStream&& other):
  _socket{other._socket},
  _frame_boundary{std::move(other._frame_boundary)}
{
  other._socket = 0;
}

CameraStream& CameraStream::operator=(CameraStream&& other) {
  _socket = other._socket;
  other._socket = 0;
  _frame_boundary = std::move(other._frame_boundary);
  return *this;
}

CameraStream::~CameraStream() {
  if (_socket) ::close(_socket);
}

void CameraStream::read_frame(std::ostream& out) {
  // Peak at the headers and find the header end position.
  char buffer[1024];
  int bytes_received = 0;
  std::size_t header_end_pos;
  std::string_view buffer_view;
  do {
    bytes_received = ::recv(_socket, (void*)buffer, sizeof(buffer), MSG_PEEK);
    if (bytes_received <= 0) break; // Error receiving!
    buffer_view =
      std::string_view{buffer, static_cast<std::size_t>(bytes_received)};
    header_end_pos = buffer_view.find("\r\n\r\n");
  } while (
    header_end_pos == std::string_view::npos &&
    bytes_received < static_cast<int>(sizeof(buffer))
  );

  if (bytes_received <= 0) {
    throw std::runtime_error{"Failed to read headers from server."};
  }
  if (!buffer_view.starts_with(_frame_boundary)) {
    throw std::runtime_error{
      "Socket stream in invalid state, expected frame boundary."
    };
  }
  if (header_end_pos == std::string_view::npos) {
    throw std::runtime_error{"Headers did not fit in initial buffer."};
  }
  std::string_view header_view = buffer_view.substr(
    _frame_boundary.size(),
    header_end_pos - _frame_boundary.size() + 4
  );

  // Parse the content length out of the headers.
  static const std::regex content_length_regex{"Content-Length:\\s+(\\d+)"};
  std::cmatch match;
  if (
    !std::regex_search(
      header_view.begin(),
      header_view.end(),
      match,
      content_length_regex
    )
  ) {
    throw std::runtime_error{
      "Streaming server did not provide a content length!"
    };
  }
  std::size_t content_length = std::stoull(match.str(1));

  // "Seek" to after the header.
  bytes_received =
    ::recv(_socket, (void*)buffer, header_end_pos + 4, /*flags=*/0);
  if (bytes_received <= 0) {
    throw std::runtime_error{"Failed to seek to end of headers."};
  }

  std::size_t content_received = 0;
  while (content_received < content_length) {
    std::size_t to_read =
      std::min(sizeof(buffer), content_length - content_received);

    bytes_received = ::recv(_socket, (void*)buffer, to_read, /*flags=*/0);
    if (bytes_received <= 0) {
      throw std::runtime_error{"Failed to read image from streaming service."};
    }
    out << std::string_view{buffer, static_cast<std::size_t>(bytes_received)};
    content_received += bytes_received;
  }

  // Read the trailing line ending after the content and throw it away.
  bytes_received = ::recv(_socket, (void*)buffer, 2, /*flags=*/0);
  if (bytes_received == 1 && *buffer == '\r') {
    bytes_received = ::recv(_socket, (void*)(buffer + 1), 1, /*flags=*/0);
    if (bytes_received == 1) bytes_received = 2;
  }
  if (bytes_received != 2 || std::string_view{buffer, 2} != "\r\n") {
    throw std::runtime_error{
      "Failed to read trailing newline after content: " +
      std::to_string(bytes_received)
    };
  }
}

}
