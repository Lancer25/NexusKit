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

struct CodecContextDeleter {
    void operator()(AVCodecContext* context) const {
        avcodec_free_context(&context);
    }
};

struct FrameDeleter {
    void operator()(AVFrame* frame) const {
        av_frame_free(&frame);
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

int frame_channel_count(const AVFrame& frame) {
#if LIBAVUTIL_VERSION_MAJOR >= 57
    return frame.ch_layout.nb_channels;
#else
    return frame.channels;
#endif
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

class MediaDecoderStorage {
public:
#if defined(NEXUS_MEDIA_WITH_FFMPEG)
    MediaDecoderStorage(
        std::unique_ptr<AVFormatContext, FormatContextDeleter> format_context,
        std::unique_ptr<AVCodecContext, CodecContextDeleter> codec_context,
        int stream_index)
        : format_context_(std::move(format_context)),
          codec_context_(std::move(codec_context)),
          stream_index_(stream_index) {}

    AVFormatContext* format_context() const {
        return format_context_.get();
    }

    AVCodecContext* codec_context() const {
        return codec_context_.get();
    }

    int stream_index() const {
        return stream_index_;
    }

    bool is_open() const {
        return format_context_ != nullptr && codec_context_ != nullptr;
    }

    void close() {
        codec_context_.reset();
        format_context_.reset();
        stream_index_ = -1;
    }
#else
    bool is_open() const {
        return false;
    }

    void close() {}
#endif

private:
#if defined(NEXUS_MEDIA_WITH_FFMPEG)
    std::unique_ptr<AVFormatContext, FormatContextDeleter> format_context_;
    std::unique_ptr<AVCodecContext, CodecContextDeleter> codec_context_;
    int stream_index_ = -1;
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

MediaDecoder::MediaDecoder() = default;

MediaDecoder::MediaDecoder(MediaDecoder&& other) noexcept = default;

MediaDecoder& MediaDecoder::operator=(MediaDecoder&& other) noexcept = default;

MediaDecoder::~MediaDecoder() = default;

Result<MediaDecoder> MediaDecoder::open(const std::filesystem::path& path) {
    const std::string operation = "Media decoder open";
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
    auto format_context = open_format_context(path, &status);
    if (!format_context) {
        diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
        return status;
    }

    const auto stream_index = av_find_best_stream(
        format_context.get(),
        AVMEDIA_TYPE_AUDIO,
        -1,
        -1,
        nullptr,
        0);
    if (stream_index < 0) {
        status = Status::not_found("media audio stream not found");
        diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
        return status;
    }

    AVStream* stream = format_context->streams[stream_index];
    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (codec == nullptr) {
        status = Status::not_found("media audio decoder not found");
        diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
        return status;
    }

    std::unique_ptr<AVCodecContext, CodecContextDeleter> codec_context(avcodec_alloc_context3(codec));
    if (!codec_context) {
        return Status::internal("FFmpeg codec context allocation failed");
    }

    const auto copy_result = avcodec_parameters_to_context(codec_context.get(), stream->codecpar);
    if (copy_result < 0) {
        status = Status::invalid_argument(
            "FFmpeg codec parameters failed: " + ffmpeg_error(copy_result));
        diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
        return status;
    }

    const auto open_result = avcodec_open2(codec_context.get(), codec, nullptr);
    if (open_result < 0) {
        status = Status::invalid_argument("FFmpeg decoder open failed: " + ffmpeg_error(open_result));
        diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
        return status;
    }

    diagnostic_log(log::Level::info, operation + " ok");
    return MediaDecoder(std::make_unique<detail::MediaDecoderStorage>(
        std::move(format_context),
        std::move(codec_context),
        stream_index));
#endif
}

bool MediaDecoder::is_open() const {
    return storage_ && storage_->is_open();
}

Result<MediaFrame> MediaDecoder::read_frame() {
    if (!is_open()) {
        return Status(StatusCode::kFailedPrecondition, "media decoder is not open");
    }

#if !defined(NEXUS_MEDIA_WITH_FFMPEG)
    return backend_unavailable_status();
#else
    std::unique_ptr<AVPacket, PacketDeleter> packet(av_packet_alloc());
    std::unique_ptr<AVFrame, FrameDeleter> frame(av_frame_alloc());
    if (!packet || !frame) {
        return Status::internal("FFmpeg decode allocation failed");
    }

    while (true) {
        const auto read_result = av_read_frame(storage_->format_context(), packet.get());
        if (read_result < 0) {
            if (read_result == AVERROR_EOF) {
                return Status::not_found("media frame stream ended");
            }
            return Status::invalid_argument("FFmpeg read packet failed: " + ffmpeg_error(read_result));
        }

        if (packet->stream_index != storage_->stream_index()) {
            av_packet_unref(packet.get());
            continue;
        }

        const auto send_result = avcodec_send_packet(storage_->codec_context(), packet.get());
        av_packet_unref(packet.get());
        if (send_result < 0) {
            return Status::invalid_argument(
                "FFmpeg decoder send failed: " + ffmpeg_error(send_result));
        }

        const auto receive_result = avcodec_receive_frame(storage_->codec_context(), frame.get());
        if (receive_result == AVERROR(EAGAIN)) {
            continue;
        }
        if (receive_result < 0) {
            return Status::invalid_argument(
                "FFmpeg decoder receive failed: " + ffmpeg_error(receive_result));
        }

        MediaFrame result;
        result.stream_index = storage_->stream_index();
        result.type = MediaStreamType::audio;
        result.pts = frame->pts == AV_NOPTS_VALUE ? 0 : frame->pts;
        result.sample_rate = frame->sample_rate;
        result.channels = frame_channel_count(*frame);

        const auto bytes_per_sample = av_get_bytes_per_sample(
            static_cast<AVSampleFormat>(frame->format));
        if (bytes_per_sample > 0 && frame->data[0] != nullptr && frame->nb_samples > 0) {
            const auto byte_count = frame->nb_samples * result.channels * bytes_per_sample;
            result.data.assign(frame->data[0], frame->data[0] + byte_count);
        }

        diagnostic_log(
            log::Level::debug,
            "Media decoder frame stream=" + std::to_string(result.stream_index) +
                " bytes=" + std::to_string(result.data.size()));
        return result;
    }
#endif
}

Status MediaDecoder::close() {
    if (storage_) {
        storage_->close();
        storage_.reset();
        diagnostic_log(log::Level::debug, "Media decoder close");
    }

    return Status::ok_status();
}

MediaDecoder::MediaDecoder(std::unique_ptr<detail::MediaDecoderStorage> storage)
    : storage_(std::move(storage)) {}

} // namespace nexus::media
