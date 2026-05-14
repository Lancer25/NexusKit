#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/media/export.h>

namespace nexus::media {

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

NEXUS_MEDIA_API FfmpegBackendInfo ffmpeg_backend_info();
NEXUS_MEDIA_API Result<MediaProbeInfo> probe_media(const std::filesystem::path& path);

} // namespace nexus::media
