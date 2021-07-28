#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <vector>

#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libavutil/imgutils.h>

#include <chrono>
#include <thread>

constexpr std::string_view DEVICE_NAME = "/dev/video0";
constexpr unsigned int WIDTH = 1920;
constexpr unsigned int HEIGHT = 1080;
constexpr ::v4l2_buf_type BUF_TYPE = V4L2_BUF_TYPE_VIDEO_CAPTURE;
constexpr ::v4l2_memory MEMORY_MODE = V4L2_MEMORY_USERPTR;

struct Buffer {
  explicit Buffer(std::size_t size): size{size}, data{std::malloc(size)} {
    std::memset(data, 0, size);
  }
  Buffer(Buffer&& other): size{other.size}, data{other.data} {
    other.size = 0;
    other.data = nullptr;
  }
  Buffer& operator=(Buffer& other) {
    size = other.size;
    data = other.data;
    other.size = 0;
    other.data = nullptr;
    return *this;
  }

  Buffer(const Buffer&) = delete;
  Buffer& operator=(const Buffer&) = delete;

  ~Buffer() {
    if (data) std::free(data);
  }

  std::size_t size;
  void* data;
};

int xioctl(int fd, int request, void* arg) {
  int res;
  do {
    res = ::ioctl(fd, request, arg);
  } while (res == -1 && errno == EINTR);
  return res;
}

int open_device() {
  struct stat status;
  if (::stat(DEVICE_NAME.data(), &status) == -1) {
    throw std::runtime_error("Failed to identify device.");
  }
  if (!S_ISCHR(status.st_mode)) {
    throw std::runtime_error("Device is not a device.");
  }
  int fd = ::open(DEVICE_NAME.data(), O_RDWR | O_NONBLOCK, 0);
  if (fd == -1) throw std::runtime_error("Failed to open device.");
  return fd;
}

std::size_t init_device(int fd) {
  ::v4l2_capability capability;
  if (xioctl(fd, VIDIOC_QUERYCAP, &capability) == -1) {
    throw std::runtime_error("Device is not a V4L2 device.");
  }
  if (capability.capabilities & V4L2_CAP_VIDEO_CAPTURE == 0) {
    throw std::runtime_error("Device cannot capture video.");
  }
  if (capability.capabilities & V4L2_CAP_STREAMING == 0) {
    throw std::runtime_error("Device does not support video streaming.");
  }

  // Reset capture crop rectangle if it is supported.
  ::v4l2_cropcap cropcap = {0};
  ::v4l2_crop crop = {0};
  cropcap.type = BUF_TYPE;
  if (xioctl(fd, VIDIOC_CROPCAP, &cropcap) == 0) {
    crop.type = BUF_TYPE;
    crop.c = cropcap.defrect;
    xioctl(fd, VIDIOC_S_CROP, &crop); // Ignore errors.
  }

  ::v4l2_format format = {0};
  format.type = BUF_TYPE;
  format.fmt.pix.width = WIDTH;
  format.fmt.pix.height = HEIGHT;
  format.fmt.pix.pixelformat = V4L2_PIX_FMT_MPEG4;
  format.fmt.pix.field = V4L2_FIELD_ANY;
  if (xioctl(fd, VIDIOC_S_FMT, &format) == -1) {
    throw std::runtime_error("Failed to set video format parameters.");
  }

  return std::max(WIDTH * 2, format.fmt.pix.bytesperline) * HEIGHT;
}

std::vector<Buffer> init_userptr(int fd, std::size_t buffer_size) {
  const unsigned int buffer_count = 4;
  ::v4l2_requestbuffers req = {0};
  req.count = buffer_count;
  req.type = BUF_TYPE;
  req.memory = MEMORY_MODE;
  if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
    throw std::runtime_error("Failed to set user pointer buffers.");
  }

  std::vector<Buffer> buffers;
  for (unsigned int i = 0; i < buffer_count; ++i) {
    buffers.emplace_back(buffer_size);
  }
  return buffers;
}

void start_capturing(int fd, std::vector<Buffer>& buffers) {
  for (std::size_t i = 0; i < buffers.size(); ++i) {
    ::v4l2_buffer buf = {0};
    buf.type = BUF_TYPE;
    buf.memory = MEMORY_MODE;
    buf.index = i;
    buf.m.userptr = reinterpret_cast<unsigned long>(buffers.at(i).data);
    buf.length = buffers.at(i).size;
    if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
      throw std::runtime_error("Failed to add buffer for streaming.");
    }
  }
  auto type = BUF_TYPE;
  if (xioctl(fd, VIDIOC_STREAMON, &type)) {
    throw std::runtime_error("Failed to start video stream.");
  }
}

void save_image(std::size_t frame_number, void* buffer, std::size_t size) {
  std::string filename = "frame-" + std::to_string(frame_number) + ".raw";
  ::FILE* fp = ::fopen(filename.c_str(), "wb");
  ::fwrite(buffer, size, 1, fp);
  ::fflush(fp);
  ::fclose(fp);

  fp = ::fopen("video.mp4", "ab");
  ::fwrite(buffer, size, 1, fp);
  ::fflush(fp);
  ::fclose(fp);
}

bool read_image(int fd, std::size_t frame_number) {
  ::v4l2_buffer buf = {0};
  buf.type = BUF_TYPE;
  buf.memory = MEMORY_MODE;
  if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
    if (errno == EAGAIN) return false;
    throw std::runtime_error(::strerror(errno));
  }
  save_image(
    frame_number,
    reinterpret_cast<void*>(buf.m.userptr),
    buf.bytesused
  );
  if (xioctl(fd, VIDIOC_QBUF, &buf)) {
    throw std::runtime_error("Failed to enqueue capture buffer.");
  }
  return true;
}

int main() {
  // TODO: Try userptr (streaming) vs readwrite

  avcodec_register_all();

  const char* filename = "video.mp4";
  const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
  if (codec == nullptr) throw std::runtime_error("MP4 codec missing.");
  AVCodecContext* context = avcodec_alloc_context3(codec);
  AVFrame* frame = av_frame_alloc();
  AVPacket* packet = av_packet_alloc();
  if (context == nullptr || frame == nullptr || packet == nullptr) {
    throw std::runtime_error("Failed to allocate FFMPEG resources.");
  }

  context->bit_rate = 400'000;
  context->width = WIDTH;
  context->height = HEIGHT;
  context->time_base = AVRational{1, 30};
  context->framerate = AVRational{30, 1};
  context->gop_size = 10; // Intra frame frequency.
  context->max_b_frames = 1;
  context->pix_fmt = AV_PIX_FMT_RGB24;
  if (avcodec_open2(context, codec, nullptr) < 0) {
    throw std::runtime_error("Failed to open codec.");
  }

  ::FILE* video_file = ::fopen(filename, "ab");
  frame->format = context->pix_fmt;
  frame->width = WIDTH;
  frame->height = HEIGHT;
  if (av_frame_get_buffer(frame, 0) < 0) {
    throw std::runtime_error("Failed to get frame buffer.");
  }

  int fd = open_device();
  std::size_t buffer_size = init_device(fd);
  std::vector<Buffer> buffers = init_userptr(fd, buffer_size);
  start_capturing(fd, buffers);

  std::this_thread::sleep_for(std::chrono::seconds(2));

  for (std::size_t i = 0; i < 30; ++i) {
    while (true) {
      ::fd_set fds;
      FD_ZERO(&fds);
      FD_SET(fd, &fds);

      ::timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 33'333;
      const int res = ::select(fd + 1, &fds, nullptr, nullptr, &tv);
      if (res == -1) {
        if (errno == EINTR) continue;
        throw std::runtime_error("Failed to select.");
      } else if (res == 0) {
        throw std::runtime_error("Select timed out.");
      }

      if (read_image(fd, i)) break;
    }
  }

  // stop_capturing();
  // uninit_device();
  // close_device();
}
