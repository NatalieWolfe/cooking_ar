from picamera import PiCamera
from time import sleep


camera = PiCamera(
  resolution=(1920, 1080),
  framerate=30
)
camera.start_preview()
camera.start_recording(
  '/home/pi/video.h264',
  level='4.2',
  intra_period=60 * 60,
  intra_refresh='cyclic',
  bitrate = 50000000 # 50Mbps

)
sleep(60)
camera.stop_recording()
camera.stop_preview()
