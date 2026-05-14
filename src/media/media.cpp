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

struct PacketDeleter {
    void operator()(AVPacket* packet) const {
        av_packet_free(&packet);
    }
};

std::string ffmpeg_error(int error_code) {
    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
    if (av_strerror(error_code, buffer, sizeof(buffer)) == 0) {
        return buffer;
    }

    return "FFmpeg error " + std::to_string(error_code);
}

std::unique_ptr<AVFormatContext, FormatContextDeleter> open_format_context(
    const std::filesystem::path& path,
    Status* status) {
    AVFormatContext* raw_context = nullptr;
    const auto open_result = avformat_open_input(&raw_context, path.string().c_str(), nullptr, nullptr);
    if (open_result < 0) {
        *status = Status::invalid_argument("FFmpeg open input failed: " + ffmpeg_error(open_result));
        return nullptr;
    }

    std::unique_ptr<AVFormatContext, FormatContextDeleter> context(raw_context);

    const auto stream_result = avformat_find_stream_info(context.get(), nullptr);
    if (stream_result < 0) {
        *status = Status::invalid_argument(
            "FFmpeg stream info failed: " + ffmpeg_error(stream_result));
        return nullptr;
    }

    return context;
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

Status validate_media_path(const std::filesystem::path& path, const std::string& operation) {
    if (path.empty()) {
        auto status = Status::invalid_argument("media path cannot be empty");
        diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
        return status;
    }

    if (!std::filesystem::exists(path)) {
        auto status = Status::not_found("media path does not exist");
        diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
        return status;
    }

    return Status::ok_status();
}

} // namespace

namespace detail {

class MediaReaderStorage {
public:
#if defined(NEXUS_MEDIA_WITH_FFMPEG)
    explicit MediaReaderStorage(std::unique_ptr<AVFormatContext, FormatContextDeleter> context)
        : context_(std::move(context)) {}

    AVFormatContext* context() const {
        return context_.get();
    }

    bool is_open() const {
        return context_ != nullptr;
    }

    void close() {
        context_.reset();
    }
#else
    bool is_open() const {
        return false;
    }

    void close() {}
#endif

private:
#if defined(NEXUS_MEDIA_WITH_FFMPEG)
    std::unique_ptr<AVFormatContext, FormatContextDeleter> context_;
#endif
};

} // namespace detail

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
    const std::string operation = "Media probe";
    diagnostic_log(log::Level::debug, operation + " path=" + path_string(path));

    const auto validation = validate_media_path(path, operation);
    if (!validation.ok()) {
        return validation;
    }

#if !defined(NEXUS_MEDIA_WITH_FFMPEG)
    auto status = backend_unavailable_status();
    diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
    return status;
#else
    Status status;
    auto context = open_format_context(path, &status);
    if (!context) {
        diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
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

MediaReader::MediaReader() = default;

MediaReader::MediaReader(MediaReader&& other) noexcept = default;

MediaReader& MediaReader::operator=(MediaReader&& other) noexcept = default;

MediaReader::~MediaReader() = default;

Result<MediaReader> MediaReader::open(const std::filesystem::path& path) {
    const std::string operation = "Media reader open";
    diagnostic_log(log::Level::debug, operation + " path=" + path_string(path));

    const auto validation = validate_media_path(path, operation);
    if (!validation.ok()) {
        return validation;
    }

#if !defined(NEXUS_MEDIA_WITH_FFMPEG)
    auto status = backend_unavailable_status();
    diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
    return status;
#else
    Status status;
    auto context = open_format_context(path, &status);
    if (!context) {
        diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
        return status;
    }

    diagnostic_log(log::Level::info, operation + " ok");
    return MediaReader(std::make_unique<detail::MediaReaderStorage>(std::move(context)));
#endif
}

bool MediaReader::is_open() const {
    return storage_ && storage_->is_open();
}

Result<MediaPacket> MediaReader::read_packet() {
    if (!is_open()) {
        return Status(StatusCode::kFailedPrecondition, "media reader is not open");
    }

#if !defined(NEXUS_MEDIA_WITH_FFMPEG)
    return backend_unavailable_status();
#else
    std::unique_ptr<AVPacket, PacketDeleter> packet(av_packet_alloc());
    if (!packet) {
        return Status::internal("FFmpeg packet allocation failed");
    }

    const auto read_result = av_read_frame(storage_->context(), packet.get());
    if (read_result < 0) {
        if (read_result == AVERROR_EOF) {
            return Status::not_found("media packet stream ended");
        }
        return Status::invalid_argument("FFmpeg read packet failed: " + ffmpeg_error(read_result));
    }

    MediaPacket result;
    result.stream_index = packet->stream_index;
    result.pts = packet->pts == AV_NOPTS_VALUE ? 0 : packet->pts;
    result.dts = packet->dts == AV_NOPTS_VALUE ? 0 : packet->dts;
    result.duration = packet->duration;
    result.key_frame = (packet->flags & AV_PKT_FLAG_KEY) != 0;
    if (packet->data != nullptr && packet->size > 0) {
        result.data.assign(packet->data, packet->data + packet->size);
    }

    diagnostic_log(
        log::Level::debug,
        "Media reader packet stream=" + std::to_string(result.stream_index) +
            " bytes=" + std::to_string(result.data.size()));
    return result;
#endif
}

Status MediaReader::close() {
    if (storage_) {
        storage_->close();
        storage_.reset();
        diagnostic_log(log::Level::debug, "Media reader close");
    }

    return Status::ok_status();
}

MediaReader::MediaReader(std::unique_ptr<detail::MediaReaderStorage> storage)
    : storage_(std::move(storage)) {}

} // namespace nexus::media
