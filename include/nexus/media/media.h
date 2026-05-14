#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/media/export.h>

namespace nexus::media {

namespace detail {
class MediaReaderStorage;
class MediaDecoderStorage;
}

struct FfmpegBackendInfo {
    std::string name = "ffmpeg";
    bool available = false;
    unsigned avutil = 0;
    unsigned avcodec = 0;
    unsigned avformat = 0;
    std::string configuration;
};

enum class MediaStreamType {
    unknown,
    video,
    audio,
    subtitle
};

struct MediaStreamInfo {
    int index = -1;
    MediaStreamType type = MediaStreamType::unknown;
    std::string codec_name;
    int width = 0;
    int height = 0;
    int sample_rate = 0;
    int channels = 0;
};

struct MediaProbeInfo {
    std::string format_name;
    std::int64_t duration_ms = 0;
    std::int64_t bit_rate = 0;
    std::vector<MediaStreamInfo> streams;
};

struct MediaPacket {
    int stream_index = -1;
    std::int64_t pts = 0;
    std::int64_t dts = 0;
    std::int64_t duration = 0;
    bool key_frame = false;
    std::vector<std::uint8_t> data;
};

struct MediaFrame {
    int stream_index = -1;
    MediaStreamType type = MediaStreamType::unknown;
    std::int64_t pts = 0;
    int width = 0;
    int height = 0;
    int sample_rate = 0;
    int channels = 0;
    std::string format_name;
    int bytes_per_sample = 0;
    bool planar = false;
    std::vector<std::uint8_t> data;
};

struct MediaDecodeOptions {
    MediaStreamType stream_type = MediaStreamType::audio;
};

enum class AudioSampleFormat {
    unknown,
    u8,
    s16,
    s32,
    flt,
    dbl
};

struct AudioConvertOptions {
    int sample_rate = 0;
    int channels = 0;
    AudioSampleFormat sample_format = AudioSampleFormat::s16;
};

enum class VideoPixelFormat {
    unknown,
    rgb24,
    rgba,
    bgr24,
    bgra
};

struct VideoConvertOptions {
    VideoPixelFormat pixel_format = VideoPixelFormat::rgb24;
};

NEXUS_MEDIA_API FfmpegBackendInfo ffmpeg_backend_info();
NEXUS_MEDIA_API Result<MediaProbeInfo> probe_media(const std::filesystem::path& path);
NEXUS_MEDIA_API Result<MediaFrame> convert_audio_frame(
    const MediaFrame& frame,
    const AudioConvertOptions& options);
NEXUS_MEDIA_API Result<MediaFrame> convert_video_frame(
    const MediaFrame& frame,
    const VideoConvertOptions& options);

class NEXUS_MEDIA_API MediaReader {
public:
    MediaReader();
    MediaReader(const MediaReader&) = delete;
    MediaReader& operator=(const MediaReader&) = delete;
    MediaReader(MediaReader&& other) noexcept;
    MediaReader& operator=(MediaReader&& other) noexcept;
    ~MediaReader();

    static Result<MediaReader> open(const std::filesystem::path& path);

    bool is_open() const;
    Result<MediaPacket> read_packet();
    Status close();

private:
    explicit MediaReader(std::unique_ptr<detail::MediaReaderStorage> storage);

    std::unique_ptr<detail::MediaReaderStorage> storage_;
};

class NEXUS_MEDIA_API MediaDecoder {
public:
    MediaDecoder();
    MediaDecoder(const MediaDecoder&) = delete;
    MediaDecoder& operator=(const MediaDecoder&) = delete;
    MediaDecoder(MediaDecoder&& other) noexcept;
    MediaDecoder& operator=(MediaDecoder&& other) noexcept;
    ~MediaDecoder();

    static Result<MediaDecoder> open(const std::filesystem::path& path);
    static Result<MediaDecoder> open(
        const std::filesystem::path& path,
        const MediaDecodeOptions& options);

    bool is_open() const;
    Result<MediaFrame> read_frame();
    Status close();

private:
    explicit MediaDecoder(std::unique_ptr<detail::MediaDecoderStorage> storage);

    std::unique_ptr<detail::MediaDecoderStorage> storage_;
};

} // namespace nexus::media
