# nexus_media

`nexus_media` contains media-processing utilities. The first implementation slice establishes the module boundary and reports whether an FFmpeg backend is linked into the build.

## Current API

- `nexus::media::FfmpegBackendInfo`: backend availability, linked FFmpeg component versions, and FFmpeg configuration text.
- `nexus::media::ffmpeg_backend_info`: returns the FFmpeg backend summary for the current build.
- `nexus::media::MediaStreamType`: stream category such as video, audio, subtitle, or unknown.
- `nexus::media::MediaStreamInfo`: stream index, type, codec name, and basic video/audio dimensions.
- `nexus::media::MediaProbeInfo`: container format name, duration, bit rate, and stream summaries.
- `nexus::media::probe_media`: validates a media path and probes container/stream metadata when FFmpeg is available.

## Scope

The initial module does not decode, encode, remux, resample, or capture media. It gives applications and tests a stable way to detect whether this NexusKit build was linked with FFmpeg targets and to request lightweight file metadata before later decoder and demuxer abstractions are added.

## Backend

When CMake targets `FFmpeg::avutil`, `FFmpeg::avcodec`, and `FFmpeg::avformat` are available while `nexus_media` is configured, the module links them privately and returns real version data. Without those targets, `ffmpeg_backend_info` returns `available == false` and zero component versions.

Public `nexus_media` headers do not expose FFmpeg headers or FFmpeg ownership types.

## Error Model

- Empty probe paths return `StatusCode::kInvalidArgument`.
- Missing probe paths return `StatusCode::kNotFound`.
- Builds without an FFmpeg backend return `StatusCode::kFailedPrecondition` for existing files.
- Invalid or unsupported media inputs return `StatusCode::kInvalidArgument` from the FFmpeg-backed parser.

## Diagnostics

Backend checks, probe start events, validation failures, unavailable-backend failures, parser failures, and successful probe summaries are written through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Without a default logger, diagnostics are silent.
