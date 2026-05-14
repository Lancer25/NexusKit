# nexus_media

`nexus_media` contains media-processing utilities. The first implementation slice establishes the module boundary and reports whether an FFmpeg backend is linked into the build.

## Current API

- `nexus::media::FfmpegBackendInfo`: backend availability, linked FFmpeg component versions, and FFmpeg configuration text.
- `nexus::media::ffmpeg_backend_info`: returns the FFmpeg backend summary for the current build.
- `nexus::media::MediaStreamType`: stream category such as video, audio, subtitle, or unknown.
- `nexus::media::MediaStreamInfo`: stream index, type, codec name, and basic video/audio dimensions.
- `nexus::media::MediaProbeInfo`: container format name, duration, bit rate, and stream summaries.
- `nexus::media::probe_media`: validates a media path and probes container/stream metadata when FFmpeg is available.
- `nexus::media::MediaPacket`: encoded packet bytes plus stream index, timestamps, duration, and key-frame flag.
- `nexus::media::MediaReader`: move-only RAII reader that opens a media file and reads encoded packets.
- `nexus::media::MediaFrame`: decoded frame metadata and bytes.
- `nexus::media::MediaDecoder`: move-only RAII decoder that opens a media file and reads decoded audio frames.

## Scope

The initial module does not encode, remux, resample, or capture media. It gives applications and tests a stable way to detect whether this NexusKit build was linked with FFmpeg targets, request lightweight file metadata, read encoded packets, and decode the first slice of audio frames before broader decoder abstractions are added.

## Backend

When CMake targets `FFmpeg::avutil`, `FFmpeg::avcodec`, and `FFmpeg::avformat` are available while `nexus_media` is configured, the module links them privately and returns real version data. Without those targets, `ffmpeg_backend_info` returns `available == false` and zero component versions.

Public `nexus_media` headers do not expose FFmpeg headers or FFmpeg ownership types.

## Error Model

- Empty probe paths return `StatusCode::kInvalidArgument`.
- Missing probe paths return `StatusCode::kNotFound`.
- Builds without an FFmpeg backend return `StatusCode::kFailedPrecondition` for existing files.
- Invalid or unsupported media inputs return `StatusCode::kInvalidArgument` from the FFmpeg-backed parser.
- Operations on closed media readers return `StatusCode::kFailedPrecondition`.
- End of packet stream returns `StatusCode::kNotFound`.
- Operations on closed media decoders return `StatusCode::kFailedPrecondition`.
- End of decoded frame stream returns `StatusCode::kNotFound`.

## Diagnostics

Backend checks, probe/reader/decoder open events, validation failures, unavailable-backend failures, parser failures, packet reads, decoded frame reads, closes, and successful probe summaries are written through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Without a default logger, diagnostics are silent.
