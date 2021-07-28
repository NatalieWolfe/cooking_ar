# Roadmap

This is a rough outline of the features needing to be implemented for this
project.

## Why not Issues?

Writing them in a list here is less overhead and I'm the only person working on
this project right now. I doubt anyone will ever even read this file besides
myself.

## Features to implement

 - Output skeleton in format compatible with FMC blender import plugin.
 - Video-based skeleton tracking.
 - Multi-person tracking.
   - Calculate deltas between frames and select lowest sum for all points.
   - Back-fill missing joints using previous frame and next.
 - Beyond stereo view position calculation.
 - Automatic camera intrinsic calibration from video.
 - Automatic camera extrinsic calibration from video with charuco cube.
 - Distributed process pipeline.
 - Skeleton bone length preservation.
