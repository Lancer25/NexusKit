# nexus_media

`nexus_media` contains media-processing utilities. The first implementation slice establishes the module boundary and reports whether an FFmpeg backend is linked into the build.

## Current API

- `nexus::media::FfmpegBackendInfo`: backend availability, linked FFmpeg component versions, and FFmpeg configuration text.
- `nexus::media::ffmpeg_backend_info`: returns the FFmpeg backend summary for the current build.

## Scope

The initial module does not decode, encode, remux, resample, or capture media. It gives applications and tests a stable way to detect whether this NexusKit build was linked with FFmpeg targets before later decoder and demuxer abstractions are added.

## Backend

When CMake targets `FFmpeg::avutil`, `FFmpeg::avcodec`, and `FFmpeg::avformat` are available while `nexus_media` is configured, the module links them privately and returns real version data. Without those targets, `ffmpeg_backend_info` returns `available == false` and zero component versions.

Public `nexus_media` headers do not expose FFmpeg headers or FFmpeg ownership types.
