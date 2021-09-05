import io

from flask import Flask, Response
from picamera import PiCamera
from uuid import uuid4


app = Flask(__name__)
camera = PiCamera(
  resolution=(1920, 1080),
  framerate=30
)


class StreamingBuffer():
  def __init__(self, camera):
    self.camera = camera
    self.frames = [None for x in range(120)]
    self.write_frame = 0
    self.read_frame = 0
    self.buffer = io.BytesIO()

  def frame(self):
    if (self.write_frame == self.read_frame): return None
    frame = self.frames[self.read_frame]
    self.read_frame = (self.read_frame + 1) % len(self.frames)
    return frame

  def write(self, buffer):
    if buffer.startswith(b'\xff\xd8'):
      # New frame! Output the current one.
      self.buffer.truncate()
      self.write_frame = (self.write_frame + 1) % len(self.frames)
      self.frames[self.write_frame] = self.buffer.getvalue()
      self.buffer.seek(0)
    return self.buffer.write(buffer)

  def __enter__(self):
    camera.start_recording(self, format='mjpeg')
    return self

  def __exit__(self, type, value, trace):
    camera.stop_recording()


def frame_generator(boundary, camera):
  format = (
    b'--%s\r\n' +
    b'Content-Type: image/jpeg\r\n' +
    b'Content-Length: %d\r\n\r\n' +
    b'%s\r\n'
  )

  with StreamingBuffer(camera) as buffer:
    while True:
      frame = buffer.frame()
      if (frame):
        yield format % (bytes(boundary, 'ascii'), len(frame), frame)
      else:
        yield ''


@app.route('/stream.mjpg')
def stream_video():
  boundary = 'frame-boundary-' + str(uuid4())
  return Response(
    frame_generator(boundary, camera),
    mimetype='multipart/x-mixed-replace; boundary=' + boundary
  )


if __name__ == '__main__':
  app.run(host='0.0.0.0')
