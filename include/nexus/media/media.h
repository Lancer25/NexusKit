#pragma once

#include <string>

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

NEXUS_MEDIA_API FfmpegBackendInfo ffmpeg_backend_info();

} // namespace nexus::media
