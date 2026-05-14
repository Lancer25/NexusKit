#include <nexus/media/media.h>

#include <filesystem>
#include <memory>
#include <string>
#include <utility>

#include <nexus/log/logger.h>

#if defined(NEXUS_MEDIA_WITH_FFMPEG)
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/avutil.h>
}
#endif

namespace nexus::media {

namespace {

Status backend_unavailable_status() {
    return Status(StatusCode::kFailedPrecondition, "FFmpeg backend is not available");
}

void diagnostic_log(log::Level level, const std::string& message) {
    log::write(level, message);
}

std::string path_string(const std::filesystem::path& path) {
    return path.string();
}

#if defined(NEXUS_MEDIA_WITH_FFMPEG)
struct FormatContextDeleter {
    void operator()(AVFormatContext* context) const {
        avformat_close_input(&context);
    }
};

std::string ffmpeg_error(int error_code) {
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    if (av_strerror(error_code, buffer, sizeof(buffer)) == 0) {
        return buffer;
    }

    return "FFmpeg error " + std::to_string(error_code);
}

MediaStreamType to_stream_type(AVMediaType type) {
    switch (type) {
    case AVMEDIA_TYPE_VIDEO:
        return MediaStreamType::video;
    case AVMEDIA_TYPE_AUDIO:
        return MediaStreamType::audio;
    case AVMEDIA_TYPE_SUBTITLE:
        return MediaStreamType::subtitle;
    default:
        return MediaStreamType::unknown;
    }
}

MediaStreamInfo to_stream_info(const AVStream& stream) {
    MediaStreamInfo info;
    info.index = static_cast<int>(stream.index);

    const auto* codec_parameters = stream.codecpar;
    if (codec_parameters == nullptr) {
        return info;
    }

    info.type = to_stream_type(codec_parameters->codec_type);
    info.codec_name = avcodec_get_name(codec_parameters->codec_id);

    if (codec_parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
        info.width = codec_parameters->width;
        info.height = codec_parameters->height;
    } else if (codec_parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
        info.sample_rate = codec_parameters->sample_rate;
#if LIBAVUTIL_VERSION_MAJOR >= 57
        info.channels = codec_parameters->ch_layout.nb_channels;
#else
        info.channels = codec_parameters->channels;
#endif
    }

    return info;
}
#endif

} // namespace

FfmpegBackendInfo ffmpeg_backend_info() {
    FfmpegBackendInfo info;

#if defined(NEXUS_MEDIA_WITH_FFMPEG)
    info.available = true;
    info.avutil = avutil_version();
    info.avcodec = avcodec_version();
    info.avformat = avformat_version();
    info.configuration = avutil_configuration();
#endif

    diagnostic_log(
        log::Level::debug,
        std::string("Media FFmpeg backend available=") + (info.available ? "true" : "false"));
    return info;
}

Result<MediaProbeInfo> probe_media(const std::filesystem::path& path) {
    diagnostic_log(log::Level::debug, "Media probe path=" + path_string(path));

    if (path.empty()) {
        auto status = Status::invalid_argument("media path cannot be empty");
        diagnostic_log(log::Level::warn, "Media probe failed: " + status.message());
        return status;
    }

    if (!std::filesystem::exists(path)) {
        auto status = Status::not_found("media path does not exist");
        diagnostic_log(log::Level::warn, "Media probe failed: " + status.message());
        return status;
    }

#if !defined(NEXUS_MEDIA_WITH_FFMPEG)
    auto status = backend_unavailable_status();
    diagnostic_log(log::Level::warn, "Media probe failed: " + status.message());
    return status;
#else
    AVFormatContext* raw_context = nullptr;
    const auto open_result = avformat_open_input(&raw_context, path.string().c_str(), nullptr, nullptr);
    if (open_result < 0) {
        auto status = Status::invalid_argument("FFmpeg open input failed: " + ffmpeg_error(open_result));
        diagnostic_log(log::Level::warn, "Media probe failed: " + status.message());
        return status;
    }

    std::unique_ptr<AVFormatContext, FormatContextDeleter> context(raw_context);

    const auto stream_result = avformat_find_stream_info(context.get(), nullptr);
    if (stream_result < 0) {
        auto status = Status::invalid_argument(
            "FFmpeg stream info failed: " + ffmpeg_error(stream_result));
        diagnostic_log(log::Level::warn, "Media probe failed: " + status.message());
        return status;
    }

    MediaProbeInfo info;
    if (context->iformat != nullptr && context->iformat->name != nullptr) {
        info.format_name = context->iformat->name;
    }
    if (context->duration != AV_NOPTS_VALUE && context->duration > 0) {
        info.duration_ms = context->duration / (AV_TIME_BASE / 1000);
    }
    if (context->bit_rate > 0) {
        info.bit_rate = context->bit_rate;
    }

    info.streams.reserve(context->nb_streams);
    for (unsigned index = 0; index < context->nb_streams; ++index) {
        if (context->streams[index] != nullptr) {
            info.streams.push_back(to_stream_info(*context->streams[index]));
        }
    }

    diagnostic_log(
        log::Level::info,
        "Media probe format=" + info.format_name +
            " streams=" + std::to_string(info.streams.size()));
    return info;
#endif
}

} // namespace nexus::media
