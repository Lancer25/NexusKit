# nexus_media

`nexus_media` provides FFmpeg-backed media processing including backend discovery, metadata probing, encoded packet reading, audio and video decoding, format conversion, encoding, container muxing, lightweight file writers, and diagnostic name helpers.

## Current API

- `nexus::media::FfmpegBackendInfo`: backend availability, linked FFmpeg component versions, and FFmpeg configuration text.
- `nexus::media::ffmpeg_backend_info`: returns the FFmpeg backend summary for the current build.
- `nexus::media::MediaStreamType`: stream category such as video, audio, subtitle, or unknown.
- `nexus::media::MediaStreamInfo`: stream index, type, codec name, and basic video/audio dimensions.
- `nexus::media::MediaProbeInfo`: container format name, duration, bit rate, and stream summaries.
- `nexus::media::probe_media`: validates a media path and probes container/stream metadata when FFmpeg is available.
- `nexus::media::MediaPacket`: encoded packet bytes plus stream index, timestamps, duration, and key-frame flag.
- `nexus::media::MediaReader`: move-only RAII reader that opens a media file and reads encoded packets.
- `nexus::media::MediaFrame`: decoded frame metadata and bytes, including audio sample format or video pixel format names when available.
- `nexus::media::MediaDecodeOptions`: decoder open options. `stream_type` defaults to audio; `stream_index` (-1) auto-selects first matching stream.
- `nexus::media::MediaDecoder`: move-only RAII decoder that opens a media file and reads decoded audio or video frames.
- `nexus::media::AudioConvertOptions` and `convert_audio_frame`: convert decoded audio frames to a target sample rate, channel count, and packed sample format.
- `nexus::media::VideoConvertOptions` and `convert_video_frame`: convert decoded video frames to RGB24, RGBA, BGR24, or BGRA.
- `nexus::media::AudioSampleFormat`: canonical audio sample format names (`u8`, `s16`, `s32`, `flt`, `dbl`, `fltp`).
- `nexus::media::VideoPixelFormat`: canonical pixel format names (`rgb24`, `rgba`, `bgr24`, `bgra`).
- `nexus::media::MediaEncodeConfig` and `MediaEncoder`: move-only RAII encoder that encodes raw audio/video frames into compressed packets.
- `nexus::media::MediaMuxConfig` and `MediaMuxer`: move-only RAII muxer that writes encoded packets to a container file (MP4, MPEG-PS, etc.).
- `nexus::media::media_stream_type_name`: return canonical names for public media stream type enum values.
- `nexus::media::audio_sample_format_name` and `video_pixel_format_name`: return canonical `MediaFrame::format_name` strings for public format enums.
- `nexus::media::write_wav_file`: write packed `s16` audio frames to a PCM WAV file.
- `nexus::media::write_ppm_file`: write RGB-family video frames to a binary PPM image.

## Scope

The current module does not provide high-level remux workflows that copy streams directly from one container to another, and it does not capture media. It gives applications and tests a stable way to detect whether this NexusKit build was linked with FFmpeg targets, request lightweight file metadata, read encoded packets, decode audio/video frames, convert decoded audio/video frames to supported formats, encode raw frames to compressed packets, mux encoded packets to container files, and write simple inspection artifacts.

Decoded audio frames include sample rate, channel count, FFmpeg sample format name, bytes per sample, and whether the decoded frame is planar. Packed audio is copied as a single interleaved byte buffer. Planar audio is copied channel-by-channel into one contiguous byte buffer.

Decoded video frames include width, height, FFmpeg pixel format name, and backend-native frame bytes. Use `convert_video_frame` when a normalized RGB-family pixel format is needed.

Audio conversion currently supports packed `u8`, `s16`, `s32`, `flt`, and `dbl` output formats. Video conversion currently supports `rgb24`, `rgba`, `bgr24`, and `bgra` output formats. Use the stream-type and format-name helpers when constructing diagnostics or checking public enum-derived names. Conversion requires an FFmpeg build that provides `swresample` and `swscale`; otherwise conversion calls return `StatusCode::kFailedPrecondition` for valid frames.

### Media Encoder

`MediaEncoder` encodes raw audio or video frames into compressed packets via FFmpeg codecs.

```cpp
nexus::media::MediaEncodeConfig config;
config.codec_name = "aac";
config.sample_rate = 44100;
config.channels = 2;
config.bit_rate = 64000;

auto encoder = nexus::media::MediaEncoder::open(config).value();
encoder.send_frame(raw_frame);       // send raw frame for encoding
auto packet = encoder.receive_packet(); // receive encoded packet
encoder.flush();                     // drain remaining packets
```

Supported audio encoders include `aac`, `libmp3lame`, `pcm_s16le`. Supported video encoders include `libx264`, `mpeg4`. Input frames must match the encoder configuration (sample rate, channels, pixel format, dimensions).

Encoder input timestamps use the encoder time base. Audio frames use `1 / sample_rate`, so `MediaFrame::pts` is measured in samples. Video frames use `1 / frame_rate`, or 25 fps when `MediaEncodeConfig::frame_rate` is 0, so `MediaFrame::pts` is measured in frame ticks. The first frame may keep the default `pts == 0`; later frames with `pts == 0` are assigned the next timestamp automatically. Set a non-zero `pts` on later frames when the caller needs explicit timestamp control.

The lightweight writers are intentionally simple inspection and test utilities. They do not encode compressed media or mux general-purpose containers; use `MediaEncoder` and `MediaMuxer` for those tasks. Use `write_wav_file` for packed PCM `s16` audio frames with matching sample metadata, and `write_ppm_file` for `rgb24`, `rgba`, `bgr24`, or `bgra` video frames when a portable image artifact is useful.

## Backend

When CMake targets `FFmpeg::avutil`, `FFmpeg::avcodec`, and `FFmpeg::avformat` are available while `nexus_media` is configured, the module links them privately and returns real version data. Without those targets, `ffmpeg_backend_info` returns `available == false` and zero component versions.

Public `nexus_media` headers do not expose FFmpeg headers or FFmpeg ownership types.

### Media Muxer

`MediaMuxer` writes encoded packets to a container file via FFmpeg's `avformat` muxing API. It supports MP4, MPEG-PS, and any container that FFmpeg can write.

```cpp
// Create an encoder first.
nexus::media::MediaEncodeConfig enc;
enc.codec_name = "aac";
enc.sample_rate = 44100;
enc.channels = 2;
enc.sample_format = nexus::media::AudioSampleFormat::fltp;
auto encoder = nexus::media::MediaEncoder::open(enc).value();

// Open a muxer (format auto-detected from extension).
auto muxer = nexus::media::MediaMuxer::open("output.mp4").value();
muxer.add_stream(enc);  // add streams before first write

// Send frames to encoder, receive packets, write to muxer.
for (int i = 0; i < num_frames; ++i) {
    encoder.send_frame(frame);
    auto pkt = encoder.receive_packet();
    if (pkt.ok()) {
        pkt->stream_index = 0;
        muxer.write_packet(*pkt);
    }
}
encoder.flush();
// drain remaining encoder packets ...
muxer.close();
```

Container format is auto-detected from the file extension (e.g., `.mp4`, `.ps`, `.mpeg`). Pass `MediaMuxConfig::format_name` to override.

Streams are added via `add_stream(encoder_config)` before the first `write_packet`. The first `write_packet` triggers the container header write (lazy header). Packets across multiple streams must be interleaved in presentation order.

`MediaPacket::pts`, `dts`, and `duration` are expressed in the muxer stream time base. For packets produced by `MediaEncoder`, use the same `MediaEncodeConfig` when adding the muxer stream and then write the packets directly after setting `stream_index`. `MediaMuxer` validates the stream index but does not rescale timestamps.

Supported audio encoders include `aac` (requires `fltp` in FFmpeg 7.x), `mp2` (MPEG-PS), `pcm_s16le`. Supported video encoders include `libx264`, `mpeg4`.

## Error Model

- Empty probe paths return `StatusCode::kInvalidArgument`.
- Missing probe paths return `StatusCode::kNotFound`.
- Builds without an FFmpeg backend return `StatusCode::kFailedPrecondition` for existing files.
- Invalid or unsupported media inputs return `StatusCode::kInvalidArgument` from the FFmpeg-backed parser.
- Operations on closed media readers return `StatusCode::kFailedPrecondition`.
- End of packet stream returns `StatusCode::kNotFound`.
- Operations on closed media decoders return `StatusCode::kFailedPrecondition`.
- End of decoded frame stream returns `StatusCode::kNotFound` after draining decoder-internal frames.
- Operations on closed media encoders return `StatusCode::kFailedPrecondition`.  End of encoded packet stream returns `StatusCode::kNotFound`.
- Operations on closed media muxers return `StatusCode::kFailedPrecondition`.  Invalid stream index returns `StatusCode::kInvalidArgument`.
- Adding a stream after the first write returns `StatusCode::kFailedPrecondition`.

All public types and functions are annotated with Doxygen `///` comments following `docs/api-style.md`.

## Diagnostics

Backend checks, probe/reader/decoder open events, validation failures, unavailable-backend failures, parser failures, packet reads, decoded frame reads, closes, and successful probe summaries are written through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Without a default logger, diagnostics are silent.
