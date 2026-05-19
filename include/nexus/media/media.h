#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/media/export.h>

/// FFmpeg-backed media probing, decoding, format conversion, and lightweight
/// frame writers.
///
/// Public headers do not expose FFmpeg types.  Users of `nexus_media` do not
/// need to link against FFmpeg directly.
namespace nexus::media {

namespace detail {
class MediaReaderStorage;
class MediaDecoderStorage;
class MediaEncoderStorage;
class MediaMuxerStorage;
}

/// FFmpeg backend build and runtime information.
struct FfmpegBackendInfo {
    /// Backend identifier.
    std::string name = "ffmpeg";
    /// True when FFmpeg libraries are linked and loaded.
    bool available = false;
    /// Linked libavutil major version, or 0 when unavailable.
    unsigned avutil = 0;
    /// Linked libavcodec major version, or 0 when unavailable.
    unsigned avcodec = 0;
    /// Linked libavformat major version, or 0 when unavailable.
    unsigned avformat = 0;
    /// FFmpeg build configuration string.
    std::string configuration;
};

/// Media stream category.
enum class MediaStreamType {
    unknown,
    video,
    audio,
    subtitle
};

/// Metadata for a single stream discovered by `probe_media`.
struct MediaStreamInfo {
    /// Stream index within the container.  -1 when unset.
    int index = -1;
    /// Stream category.
    MediaStreamType type = MediaStreamType::unknown;
    /// Codec short name from the demuxer.
    std::string codec_name;
    /// Video width in pixels.  0 for non-video streams.
    int width = 0;
    /// Video height in pixels.  0 for non-video streams.
    int height = 0;
    /// Audio sample rate in Hz.  0 for non-audio streams.
    int sample_rate = 0;
    /// Audio channel count.  0 for non-audio streams.
    int channels = 0;
};

/// Container-level metadata from `probe_media`.
struct MediaProbeInfo {
    /// Container format short name.
    std::string format_name;
    /// Duration in milliseconds.  0 when unavailable.
    std::int64_t duration_ms = 0;
    /// Bit rate in bits per second.  0 when unavailable.
    std::int64_t bit_rate = 0;
    /// Stream list discovered by the probe.
    std::vector<MediaStreamInfo> streams;
};

/// A single encoded packet read from a `MediaReader`.
struct MediaPacket {
    /// Originating stream index.  -1 when unset.
    int stream_index = -1;
    /// Presentation timestamp in stream time-base units.
    std::int64_t pts = 0;
    /// Decode timestamp in stream time-base units.
    std::int64_t dts = 0;
    /// Packet duration in stream time-base units.  0 when unknown.
    std::int64_t duration = 0;
    /// True when this packet is an independent key frame.
    bool key_frame = false;
    /// Encoded packet bytes.
    std::vector<std::uint8_t> data;
};

/// A decoded audio or video frame.
///
/// Audio frames: `sample_rate`, `channels`, `format_name` (FFmpeg sample
/// format), `bytes_per_sample`, and `planar` describe the data layout.
/// Packed audio is stored as one contiguous interleaved buffer.
/// Planar audio is stored channel-by-channel in one contiguous buffer.
///
/// Video frames: `width`, `height`, and `format_name` (FFmpeg pixel format)
/// describe the data layout.  Frame bytes are backend-native; use
/// `convert_video_frame` for a normalized RGB-family format.
struct MediaFrame {
    /// Originating stream index.  -1 when unset.
    int stream_index = -1;
    /// Audio or video frame type.
    MediaStreamType type = MediaStreamType::unknown;
    /// Presentation timestamp in encoder input time-base units.
    ///
    /// For encoder input, audio uses `1 / sample_rate` and video uses
    /// `1 / frame_rate` (or 25 fps when `frame_rate` is 0).  The first
    /// frame may keep the default 0 timestamp.  Later frames with `pts == 0`
    /// are auto-timestamped after the previous frame; set a non-zero value
    /// when explicit caller timestamps are required.
    std::int64_t pts = 0;
    /// Video width in pixels.  0 for audio frames.
    int width = 0;
    /// Video height in pixels.  0 for audio frames.
    int height = 0;
    /// Audio sample rate in Hz.  0 for video frames.
    int sample_rate = 0;
    /// Audio channel count.  0 for video frames.
    int channels = 0;
    /// Backend format name (FFmpeg sample or pixel format string).
    std::string format_name;
    /// Bytes per audio sample.  0 for video frames or when unknown.
    int bytes_per_sample = 0;
    /// True when the audio data is planar (separate channel buffers).
    bool planar = false;
    /// Frame bytes.
    std::vector<std::uint8_t> data;
};

/// Options for `MediaDecoder::open`.
struct MediaDecodeOptions {
    /// Stream type to select for decoding.  Defaults to audio.
    MediaStreamType stream_type = MediaStreamType::audio;
    /// Specific stream index.  -1 (default) auto-selects the first matching
    /// stream of `stream_type`.  Set to a non-negative value to target a
    /// particular stream.
    int stream_index = -1;
};

/// Audio sample formats supported by `convert_audio_frame`.
enum class AudioSampleFormat {
    unknown,
    u8,
    s16,
    s32,
    flt,
    dbl,
    fltp
};

/// Target parameters for `convert_audio_frame`.
///
/// Set `sample_rate` or `channels` to 0 to keep the source value.
struct AudioConvertOptions {
    /// Target sample rate.  0 keeps the source sample rate.
    int sample_rate = 0;
    /// Target channel count.  0 keeps the source channel count.
    int channels = 0;
    /// Target sample format.
    AudioSampleFormat sample_format = AudioSampleFormat::s16;
};

/// RGB-family pixel formats supported by `convert_video_frame`.
enum class VideoPixelFormat {
    unknown,
    rgb24,
    rgba,
    bgr24,
    bgra
};

/// Target parameters for `convert_video_frame`.
struct VideoConvertOptions {
    /// Target pixel format.
    VideoPixelFormat pixel_format = VideoPixelFormat::rgb24;
};

/// Configuration for `MediaEncoder::open`.
struct MediaEncodeConfig {
    /// FFmpeg encoder name, e.g. "aac", "libmp3lame", "libx264".
    std::string codec_name;
    /// For audio: sample rate in Hz. Required for audio.
    int sample_rate = 0;
    /// For audio: channel count. Required for audio.
    int channels = 0;
    /// For video: frame width in pixels. Required for video.
    int width = 0;
    /// For video: frame height in pixels. Required for video.
    int height = 0;
    /// Target bit rate in bits per second. 0 uses the codec default.
    std::int64_t bit_rate = 0;
    /// For video: frames per second. 0 uses 25 fps.
    int frame_rate = 0;
    /// Audio sample format for encoder input.
    /// Default: s16.
    AudioSampleFormat sample_format = AudioSampleFormat::s16;
    /// Video pixel format for encoder input, e.g. "yuv420p".
    /// Empty uses the codec default.
    std::string pixel_format;
};

/// Move-only RAII media encoder.
///
/// Encodes raw audio or video frames into compressed packets via FFmpeg
/// codecs.  Created via `open(config)`.  Copy is deleted; move transfers
/// ownership.  A moved-from encoder is closed and `is_open()` returns false.
///
/// Intended for single-threaded use.
class NEXUS_MEDIA_API MediaEncoder {
public:
    /// Constructs a closed encoder.
    MediaEncoder();
    MediaEncoder(const MediaEncoder&) = delete;
    MediaEncoder& operator=(const MediaEncoder&) = delete;
    MediaEncoder(MediaEncoder&& other) noexcept;
    MediaEncoder& operator=(MediaEncoder&& other) noexcept;
    ~MediaEncoder();

    /// Opens an encoder with the given configuration.
    ///
    /// @return An open encoder on success.
    /// @retval kInvalidArgument when required config fields are missing.
    /// @retval kFailedPrecondition when FFmpeg is unavailable or the codec
    /// cannot be opened.
    static Result<MediaEncoder> open(const MediaEncodeConfig& config);

    /// True when the encoder is open.
    bool is_open() const;

    /// Sends a raw frame for encoding.
    ///
    /// Frame timestamps use the encoder input time base.  Audio frames are
    /// measured in samples.  Video frames are measured in frame ticks.  After
    /// the first frame, a frame with `pts == 0` continues from the previous
    /// input timestamp automatically.
    ///
    /// May return `kResourceExhausted` (EAGAIN); call `receive_packet()`
    /// to drain output before sending more frames.
    ///
    /// @retval kFailedPrecondition when the encoder is closed.
    /// @retval kInvalidArgument when the frame metadata does not match the
    /// encoder configuration.
    Status send_frame(const MediaFrame& frame);

    /// Receives the next encoded packet.
    ///
    /// @return The next encoded packet on success.
    /// @retval kNotFound when no more packets are available.
    /// @retval kFailedPrecondition when the encoder is closed.
    Result<MediaPacket> receive_packet();

    /// Flushes remaining encoded packets from the encoder.
    ///
    /// Call `receive_packet()` after this until it returns kNotFound.
    ///
    /// @retval kFailedPrecondition when the encoder is closed.
    Status flush();

    /// Closes the encoder.  Idempotent.
    Status close();

private:
    explicit MediaEncoder(std::unique_ptr<detail::MediaEncoderStorage> storage);

    std::unique_ptr<detail::MediaEncoderStorage> storage_;
};

/// Configuration for `MediaMuxer::open`.
struct MediaMuxConfig {
    /// Container format name, e.g. "mp4", "mpeg".  Auto-detected from file
    /// extension when empty.
    std::string format_name;
};

/// Move-only RAII media muxer.
///
/// Writes encoded packets into a container file via FFmpeg's muxing API.
/// Created via `open(path)`.  Copy is deleted; move transfers ownership.
/// A moved-from muxer is closed and `is_open()` returns false.
///
/// Streams are added via `add_stream` before the first `write_packet` call.
/// Packets must be interleaved in presentation order across streams.
///
/// Intended for single-threaded use.
class NEXUS_MEDIA_API MediaMuxer {
public:
    /// Constructs a closed muxer.
    MediaMuxer();
    MediaMuxer(const MediaMuxer&) = delete;
    MediaMuxer& operator=(const MediaMuxer&) = delete;
    MediaMuxer(MediaMuxer&& other) noexcept;
    MediaMuxer& operator=(MediaMuxer&& other) noexcept;
    ~MediaMuxer();

    /// Opens a muxer for writing to `path`.
    ///
    /// @return An open muxer on success.
    /// @retval kInvalidArgument when the format is not recognised.
    /// @retval kFailedPrecondition when FFmpeg is unavailable.
    static Result<MediaMuxer> open(const std::filesystem::path& path,
                                   const MediaMuxConfig& config = {});

    /// Adds a stream with encoder configuration, returns the stream index.
    ///
    /// Must be called before the first `write_packet`.
    ///
    /// @retval kFailedPrecondition when the muxer is closed or the header has
    /// already been written.
    Result<int> add_stream(const MediaEncodeConfig& encoder_config);

    /// Writes an encoded packet to the container.
    ///
    /// The first call triggers the container header write.
    /// Packet timestamps are in the stream time base created by
    /// `add_stream`.  Packets from `MediaEncoder` can be written directly
    /// when the muxer stream was added with the same `MediaEncodeConfig`.
    /// Packets must be interleaved in presentation order.
    ///
    /// @retval kFailedPrecondition when the muxer is closed.
    Status write_packet(const MediaPacket& packet);

    /// Finalizes the container (writes trailer).  Idempotent.
    Status close();

    /// True when the muxer is open.
    bool is_open() const;

private:
    explicit MediaMuxer(std::unique_ptr<detail::MediaMuxerStorage> storage);

    std::unique_ptr<detail::MediaMuxerStorage> storage_;
};

/// Returns a canonical name for a `MediaStreamType` value.
NEXUS_MEDIA_API std::string media_stream_type_name(MediaStreamType type);

/// Returns the canonical `MediaFrame::format_name` for an `AudioSampleFormat`.
NEXUS_MEDIA_API std::string audio_sample_format_name(AudioSampleFormat format);

/// Returns the canonical `MediaFrame::format_name` for a `VideoPixelFormat`.
NEXUS_MEDIA_API std::string video_pixel_format_name(VideoPixelFormat format);

/// Returns FFmpeg backend build/runtime information.
///
/// Component versions are zero and `available` is false when this build of
/// `nexus_media` was configured without FFmpeg targets.
NEXUS_MEDIA_API FfmpegBackendInfo ffmpeg_backend_info();

/// Probes a media file and returns container and stream metadata.
///
/// @return The probe result on success.
/// @retval kInvalidArgument when `path` is empty.
/// @retval kNotFound when `path` does not exist on disk.
/// @retval kFailedPrecondition when FFmpeg is unavailable or the file cannot
/// be opened.
/// @retval kInvalidArgument when the file is not a recognized media container.
///
/// This function opens the file, reads the container header, and closes it
/// immediately.  It may block on disk I/O.
NEXUS_MEDIA_API Result<MediaProbeInfo> probe_media(const std::filesystem::path& path);

/// Converts a decoded audio frame to the target sample rate, channel count,
/// and sample format.
///
/// Uses libswresample internally.  Returns `kFailedPrecondition` when FFmpeg
/// or swresample is unavailable.
///
/// @param frame Source decoded audio frame.
/// @param options Target parameters.  Set `sample_rate` or `channels` to 0 to
/// preserve the source value.
/// @return A packed output frame with interleaved data.
NEXUS_MEDIA_API Result<MediaFrame> convert_audio_frame(
    const MediaFrame& frame,
    const AudioConvertOptions& options);

/// Converts a decoded video frame to a standard RGB-family pixel format.
///
/// Uses libswscale internally.  Returns `kFailedPrecondition` when FFmpeg or
/// swscale is unavailable.
///
/// @param frame Source decoded video frame.
/// @param options Target pixel format.
/// @return A converted frame with contiguous rows (`width * bpp`).
NEXUS_MEDIA_API Result<MediaFrame> convert_video_frame(
    const MediaFrame& frame,
    const VideoConvertOptions& options);

/// Writes a sequence of packed `s16` audio frames to a PCM WAV file.
///
/// This is a lightweight inspection and test utility.  It does not encode
/// compressed audio or mux general-purpose containers.
///
/// @return `kInvalidArgument` when `frames` is empty or any frame has
/// mismatched sample metadata.  `kInternal` on file write errors.
NEXUS_MEDIA_API Status write_wav_file(
    const std::filesystem::path& path,
    const std::vector<MediaFrame>& frames);

/// Copies media streams from `source` to `dest` without re-encoding.
///
/// Opens the source file, copies all streams, and writes packets directly
/// without decoding or encoding.  Equivalent to `ffmpeg -c copy`.
///
/// @retval kInvalidArgument when source or dest path is empty.
/// @retval kNotFound when source does not exist.
/// @retval kUnavailable when FFmpeg is unavailable or the operation fails.
NEXUS_MEDIA_API Status remux_file(
    const std::filesystem::path& source,
    const std::filesystem::path& dest);

/// Writes a video frame to a binary P6 PPM image file.
///
/// Supports `rgb24`, `rgba`, `bgr24`, and `bgra` frames.  Alpha is dropped
/// when present.  This is a lightweight inspection and test utility.
///
/// @return `kInvalidArgument` when the frame is not an RGB-family format.
/// `kInternal` on file write errors.
NEXUS_MEDIA_API Status write_ppm_file(
    const std::filesystem::path& path,
    const MediaFrame& frame);

/// Move-only RAII reader that reads encoded packets from a media file.
///
/// Created via `open(path)`.  Copy is deleted; move transfers ownership.
/// A moved-from reader is closed and `is_open()` returns false.
///
/// Intended for single-threaded use.
class NEXUS_MEDIA_API MediaReader {
public:
    /// Constructs a closed reader.
    MediaReader();
    MediaReader(const MediaReader&) = delete;
    MediaReader& operator=(const MediaReader&) = delete;
    MediaReader(MediaReader&& other) noexcept;
    MediaReader& operator=(MediaReader&& other) noexcept;
    /// Closes the reader if open.
    ~MediaReader();

    /// Opens a media file for packet reading.
    ///
    /// May block on disk I/O.
    ///
    /// @return An open reader on success.
    /// @retval kInvalidArgument when `path` is empty.
    /// @retval kNotFound when `path` does not exist.
    /// @retval kFailedPrecondition when FFmpeg is unavailable or the file
    /// cannot be opened.
    static Result<MediaReader> open(const std::filesystem::path& path);

    /// True when the reader is open and ready to read packets.
    bool is_open() const;

    /// Reads the next encoded packet from the file.
    ///
    /// May block on disk I/O or demuxer buffering.
    ///
    /// @return The next packet on success.
    /// @retval kFailedPrecondition when the reader is closed.
    /// @retval kNotFound when the end of the packet stream is reached.
    Result<MediaPacket> read_packet();

    /// Closes the reader.  Idempotent; safe to call on a closed reader.
    Status close();

private:
    explicit MediaReader(std::unique_ptr<detail::MediaReaderStorage> storage);

    std::unique_ptr<detail::MediaReaderStorage> storage_;
};

/// Move-only RAII decoder that reads decoded frames from a media file.
///
/// Created via `open(path)` or `open(path, options)`.  Copy is deleted; move
/// transfers ownership.  A moved-from decoder is closed and `is_open()`
/// returns false.
///
/// The overload that accepts `MediaDecodeOptions` selects the first stream
/// matching the requested `stream_type`.
///
/// Intended for single-threaded use.
class NEXUS_MEDIA_API MediaDecoder {
public:
    /// Constructs a closed decoder.
    MediaDecoder();
    MediaDecoder(const MediaDecoder&) = delete;
    MediaDecoder& operator=(const MediaDecoder&) = delete;
    MediaDecoder(MediaDecoder&& other) noexcept;
    MediaDecoder& operator=(MediaDecoder&& other) noexcept;
    /// Closes the decoder if open.
    ~MediaDecoder();

    /// Opens a media file for frame decoding.  Selects the first audio stream.
    ///
    /// May block on disk I/O and codec initialization.
    ///
    /// @return An open decoder on success.
    /// @retval kInvalidArgument when `path` is empty.
    /// @retval kNotFound when `path` does not exist or no matching stream was
    /// found.
    /// @retval kFailedPrecondition when FFmpeg is unavailable or the file
    /// cannot be opened/decoded.
    static Result<MediaDecoder> open(const std::filesystem::path& path);

    /// Opens a media file for frame decoding, selecting the first stream
    /// matching `options.stream_type`.
    ///
    /// May block on disk I/O and codec initialization.
    ///
    /// @return An open decoder on success, or the same error codes as above.
    static Result<MediaDecoder> open(
        const std::filesystem::path& path,
        const MediaDecodeOptions& options);

    /// True when the decoder is open and ready to read frames.
    bool is_open() const;

    /// Reads the next decoded frame.
    ///
    /// May block on disk I/O and decoding.
    ///
    /// @return The next decoded frame on success.
    /// @retval kFailedPrecondition when the decoder is closed.
    /// @retval kNotFound when the end of the frame stream is reached.
    Result<MediaFrame> read_frame();

    /// Closes the decoder.  Idempotent; safe to call on a closed decoder.
    Status close();

private:
    explicit MediaDecoder(std::unique_ptr<detail::MediaDecoderStorage> storage);

    std::unique_ptr<detail::MediaDecoderStorage> storage_;
};

} // namespace nexus::media
