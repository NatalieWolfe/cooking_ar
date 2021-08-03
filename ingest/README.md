# AR Ingestion Pipeline

```
Legend:
 [endpoint]
 (service)
  --> message queue

     [input]
        |
        V
    (session) ----------> (video)
        |                    |
        V                    V
  (frame coord) <------ (extractor)
        |            |       |
        V            |       V
   (projector) --> (ambiguous frames)
        |
        V
   (animator)
        |
        V
    [output]
```

## Services

### Session

Entry point to the ingestion pipeline, this service is where source video files
are uploaded along with the camera calibration data.

### Video

This service takes a video file and slices it into individual frames. Each frame
is paired with its timing offset from the synchronization frame.

### Extractor

Processes each frame to extract the pose information. Frames with poor
extraction results are sent to the Ambiguous Frames service to be cleaned up.
Good frames are sent directly to the Frame Coordinator service.

### Frame Coordinator

Accumulates processed frames until the correlated frames from every camera in
the session have been processed. Once all correlated frames for a timepoint have
been processed, the posses are passed on as a bundle.

### Projector

Projects 2d poses from all cameras into the 3d positions. Any frames with poor
extraction results are sent to the Ambiguous Frames service to be cleaned up.
Good frames are sent to the Animator service.

### Ambiguous Frames

Any frames which cannot be resolved well (low confidence from OpenPose or mixed
up pose IDs between cameras) are sent here. They are collected to be hand fixed
by a human and then sent back to the Frame Coordinator service.

### Animator

Collects all the projected 3d poses and organizes them into key frames for
animations to be imported into Blender.
