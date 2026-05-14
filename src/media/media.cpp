#include <nexus/media/media.h>

#include <filesystem>
#include <fstream>
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
#include <libavutil/imgutils.h>
#include <libavutil/pixdesc.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
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

void write_u16_le(std::ostream& output, std::uint16_t value) {
    output.put(static_cast<char>(value & 0xff));
    output.put(static_cast<char>((value >> 8) & 0xff));
}

void write_u32_le(std::ostream& output, std::uint32_t value) {
    output.put(static_cast<char>(value & 0xff));
    output.put(static_cast<char>((value >> 8) & 0xff));
    output.put(static_cast<char>((value >> 16) & 0xff));
    output.put(static_cast<char>((value >> 24) & 0xff));
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

struct SwrContextDeleter {
    void operator()(SwrContext* context) const {
        swr_free(&context);
    }
};

struct SwsContextDeleter {
    void operator()(SwsContext* context) const {
        sws_freeContext(context);
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

AVMediaType to_av_media_type(MediaStreamType type) {
    switch (type) {
    case MediaStreamType::video:
        return AVMEDIA_TYPE_VIDEO;
    case MediaStreamType::audio:
        return AVMEDIA_TYPE_AUDIO;
    case MediaStreamType::subtitle:
        return AVMEDIA_TYPE_SUBTITLE;
    case MediaStreamType::unknown:
    default:
        return AVMEDIA_TYPE_UNKNOWN;
    }
}

AVSampleFormat to_av_sample_format(AudioSampleFormat format) {
    switch (format) {
    case AudioSampleFormat::u8:
        return AV_SAMPLE_FMT_U8;
    case AudioSampleFormat::s16:
        return AV_SAMPLE_FMT_S16;
    case AudioSampleFormat::s32:
        return AV_SAMPLE_FMT_S32;
    case AudioSampleFormat::flt:
        return AV_SAMPLE_FMT_FLT;
    case AudioSampleFormat::dbl:
        return AV_SAMPLE_FMT_DBL;
    case AudioSampleFormat::unknown:
    default:
        return AV_SAMPLE_FMT_NONE;
    }
}

AVSampleFormat sample_format_from_name(const std::string& name) {
    const auto format = av_get_sample_fmt(name.c_str());
    return format == AV_SAMPLE_FMT_NONE ? AV_SAMPLE_FMT_NONE : format;
}

AVPixelFormat to_av_pixel_format(VideoPixelFormat format) {
    switch (format) {
    case VideoPixelFormat::rgb24:
        return AV_PIX_FMT_RGB24;
    case VideoPixelFormat::rgba:
        return AV_PIX_FMT_RGBA;
    case VideoPixelFormat::bgr24:
        return AV_PIX_FMT_BGR24;
    case VideoPixelFormat::bgra:
        return AV_PIX_FMT_BGRA;
    case VideoPixelFormat::unknown:
    default:
        return AV_PIX_FMT_NONE;
    }
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
        int stream_index,
        MediaStreamType stream_type)
        : format_context_(std::move(format_context)),
          codec_context_(std::move(codec_context)),
          stream_index_(stream_index),
          stream_type_(stream_type) {}

    AVFormatContext* format_context() const {
        return format_context_.get();
    }

    AVCodecContext* codec_context() const {
        return codec_context_.get();
    }

    int stream_index() const {
        return stream_index_;
    }

    MediaStreamType stream_type() const {
        return stream_type_;
    }

    bool is_open() const {
        return format_context_ != nullptr && codec_context_ != nullptr;
    }

    void close() {
        codec_context_.reset();
        format_context_.reset();
        stream_index_ = -1;
        stream_type_ = MediaStreamType::unknown;
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
    MediaStreamType stream_type_ = MediaStreamType::unknown;
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

Result<MediaFrame> convert_audio_frame(
    const MediaFrame& frame,
    const AudioConvertOptions& options) {
    if (frame.type != MediaStreamType::audio) {
        return Status::invalid_argument("media frame is not audio");
    }
    if (frame.data.empty()) {
        return Status::invalid_argument("audio frame data cannot be empty");
    }
    if (frame.sample_rate <= 0 || frame.channels <= 0 || frame.bytes_per_sample <= 0) {
        return Status::invalid_argument("audio frame metadata is incomplete");
    }
    if (options.sample_rate <= 0 || options.channels <= 0) {
        return Status::invalid_argument("audio conversion options are invalid");
    }

#if !defined(NEXUS_MEDIA_WITH_FFMPEG)
    static_cast<void>(options);
    return backend_unavailable_status();
#else
    const auto input_format = sample_format_from_name(frame.format_name);
    const auto output_format = to_av_sample_format(options.sample_format);
    if (input_format == AV_SAMPLE_FMT_NONE || output_format == AV_SAMPLE_FMT_NONE) {
        return Status::invalid_argument("audio sample format is not supported");
    }

    const auto input_samples = static_cast<int>(
        frame.data.size() /
        static_cast<std::size_t>(frame.channels * frame.bytes_per_sample));
    if (input_samples <= 0) {
        return Status::invalid_argument("audio frame sample count is invalid");
    }

    AVChannelLayout input_layout;
    AVChannelLayout output_layout;
    av_channel_layout_default(&input_layout, frame.channels);
    av_channel_layout_default(&output_layout, options.channels);

    SwrContext* raw_context = nullptr;
    const auto alloc_result = swr_alloc_set_opts2(
        &raw_context,
        &output_layout,
        output_format,
        options.sample_rate,
        &input_layout,
        input_format,
        frame.sample_rate,
        0,
        nullptr);
    av_channel_layout_uninit(&input_layout);
    av_channel_layout_uninit(&output_layout);
    if (alloc_result < 0) {
        return Status::invalid_argument("FFmpeg audio converter allocation failed: " + ffmpeg_error(alloc_result));
    }

    std::unique_ptr<SwrContext, SwrContextDeleter> context(raw_context);
    const auto init_result = swr_init(context.get());
    if (init_result < 0) {
        return Status::invalid_argument("FFmpeg audio converter init failed: " + ffmpeg_error(init_result));
    }

    std::vector<const std::uint8_t*> input_planes;
    if (frame.planar) {
        const auto plane_size = static_cast<std::size_t>(input_samples * frame.bytes_per_sample);
        input_planes.reserve(static_cast<std::size_t>(frame.channels));
        for (int channel = 0; channel < frame.channels; ++channel) {
            input_planes.push_back(frame.data.data() + plane_size * static_cast<std::size_t>(channel));
        }
    } else {
        input_planes.push_back(frame.data.data());
    }

    const auto delayed_samples = swr_get_delay(context.get(), frame.sample_rate);
    const auto output_samples = static_cast<int>(av_rescale_rnd(
        delayed_samples + input_samples,
        options.sample_rate,
        frame.sample_rate,
        AV_ROUND_UP));
    if (output_samples <= 0) {
        return Status::invalid_argument("FFmpeg audio converter output size is invalid");
    }

    const auto output_bytes_per_sample = av_get_bytes_per_sample(output_format);
    const auto output_byte_count = output_samples * options.channels * output_bytes_per_sample;
    std::vector<std::uint8_t> output(static_cast<std::size_t>(output_byte_count));
    std::uint8_t* output_planes[] = {output.data()};

    const auto converted_samples = swr_convert(
        context.get(),
        output_planes,
        output_samples,
        input_planes.data(),
        input_samples);
    if (converted_samples < 0) {
        return Status::invalid_argument("FFmpeg audio converter failed: " + ffmpeg_error(converted_samples));
    }

    auto total_samples = converted_samples;
    output.resize(static_cast<std::size_t>(
        total_samples * options.channels * output_bytes_per_sample));

    const auto flush_capacity = static_cast<int>(swr_get_delay(context.get(), options.sample_rate));
    if (flush_capacity > 0) {
        const auto old_size = output.size();
        output.resize(old_size + static_cast<std::size_t>(
            flush_capacity * options.channels * output_bytes_per_sample));
        std::uint8_t* flush_planes[] = {output.data() + old_size};
        const auto flushed_samples = swr_convert(
            context.get(),
            flush_planes,
            flush_capacity,
            nullptr,
            0);
        if (flushed_samples < 0) {
            return Status::invalid_argument(
                "FFmpeg audio converter flush failed: " + ffmpeg_error(flushed_samples));
        }
        total_samples += flushed_samples;
        output.resize(static_cast<std::size_t>(
            total_samples * options.channels * output_bytes_per_sample));
    }

    MediaFrame converted;
    converted.stream_index = frame.stream_index;
    converted.type = MediaStreamType::audio;
    converted.pts = frame.pts;
    converted.sample_rate = options.sample_rate;
    converted.channels = options.channels;
    const char* output_name = av_get_sample_fmt_name(output_format);
    converted.format_name = output_name == nullptr ? "" : output_name;
    converted.bytes_per_sample = output_bytes_per_sample;
    converted.planar = false;
    converted.data = std::move(output);
    return converted;
#endif
}

Result<MediaFrame> convert_video_frame(
    const MediaFrame& frame,
    const VideoConvertOptions& options) {
    if (frame.type != MediaStreamType::video) {
        return Status::invalid_argument("media frame is not video");
    }
    if (frame.data.empty()) {
        return Status::invalid_argument("video frame data cannot be empty");
    }
    if (frame.width <= 0 || frame.height <= 0 || frame.format_name.empty()) {
        return Status::invalid_argument("video frame metadata is incomplete");
    }

#if !defined(NEXUS_MEDIA_WITH_FFMPEG)
    static_cast<void>(options);
    return backend_unavailable_status();
#else
    const auto input_format = av_get_pix_fmt(frame.format_name.c_str());
    const auto output_format = to_av_pixel_format(options.pixel_format);
    if (input_format == AV_PIX_FMT_NONE || output_format == AV_PIX_FMT_NONE) {
        return Status::invalid_argument("video pixel format is not supported");
    }

    std::uint8_t* input_data[4] = {};
    int input_linesize[4] = {};
    const auto input_fill_result = av_image_fill_arrays(
        input_data,
        input_linesize,
        frame.data.data(),
        input_format,
        frame.width,
        frame.height,
        1);
    if (input_fill_result < 0) {
        return Status::invalid_argument(
            "FFmpeg image input fill failed: " + ffmpeg_error(input_fill_result));
    }

    const auto output_buffer_size =
        av_image_get_buffer_size(output_format, frame.width, frame.height, 1);
    if (output_buffer_size < 0) {
        return Status::invalid_argument(
            "FFmpeg image output buffer size failed: " + ffmpeg_error(output_buffer_size));
    }

    std::vector<std::uint8_t> output(static_cast<std::size_t>(output_buffer_size));
    std::uint8_t* output_data[4] = {};
    int output_linesize[4] = {};
    const auto output_fill_result = av_image_fill_arrays(
        output_data,
        output_linesize,
        output.data(),
        output_format,
        frame.width,
        frame.height,
        1);
    if (output_fill_result < 0) {
        return Status::invalid_argument(
            "FFmpeg image output fill failed: " + ffmpeg_error(output_fill_result));
    }

    std::unique_ptr<SwsContext, SwsContextDeleter> context(sws_getContext(
        frame.width,
        frame.height,
        input_format,
        frame.width,
        frame.height,
        output_format,
        SWS_BILINEAR,
        nullptr,
        nullptr,
        nullptr));
    if (!context) {
        return Status::internal("FFmpeg video converter allocation failed");
    }

    const auto scaled_height = sws_scale(
        context.get(),
        input_data,
        input_linesize,
        0,
        frame.height,
        output_data,
        output_linesize);
    if (scaled_height != frame.height) {
        return Status::internal("FFmpeg video converter produced incomplete frame");
    }

    MediaFrame converted;
    converted.stream_index = frame.stream_index;
    converted.type = MediaStreamType::video;
    converted.pts = frame.pts;
    converted.width = frame.width;
    converted.height = frame.height;
    const char* output_name = av_get_pix_fmt_name(output_format);
    converted.format_name = output_name == nullptr ? "" : output_name;
    converted.data = std::move(output);
    return converted;
#endif
}

Status write_wav_file(
    const std::filesystem::path& path,
    const std::vector<MediaFrame>& frames) {
    if (path.empty()) {
        return Status::invalid_argument("WAV path cannot be empty");
    }
    if (frames.empty()) {
        return Status::invalid_argument("WAV frames cannot be empty");
    }

    const auto& first = frames.front();
    if (first.type != MediaStreamType::audio ||
        first.sample_rate <= 0 ||
        first.channels <= 0 ||
        first.bytes_per_sample <= 0 ||
        first.planar ||
        first.format_name != "s16") {
        return Status::invalid_argument("WAV writer requires packed s16 audio frames");
    }

    std::uint64_t data_size = 0;
    for (const auto& frame : frames) {
        if (frame.type != MediaStreamType::audio ||
            frame.sample_rate != first.sample_rate ||
            frame.channels != first.channels ||
            frame.bytes_per_sample != first.bytes_per_sample ||
            frame.planar != first.planar ||
            frame.format_name != first.format_name ||
            frame.data.empty()) {
            return Status::invalid_argument("WAV frames must be non-empty matching packed s16 audio");
        }
        data_size += frame.data.size();
    }

    if (data_size > 0xffffffffull - 36ull) {
        return Status(StatusCode::kResourceExhausted, "WAV data is too large");
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return Status::invalid_argument("WAV path cannot be opened");
    }

    const auto bits_per_sample = static_cast<std::uint16_t>(first.bytes_per_sample * 8);
    const auto block_align = static_cast<std::uint16_t>(first.channels * first.bytes_per_sample);
    const auto byte_rate = static_cast<std::uint32_t>(first.sample_rate * block_align);

    output.write("RIFF", 4);
    write_u32_le(output, static_cast<std::uint32_t>(36ull + data_size));
    output.write("WAVE", 4);
    output.write("fmt ", 4);
    write_u32_le(output, 16);
    write_u16_le(output, 1);
    write_u16_le(output, static_cast<std::uint16_t>(first.channels));
    write_u32_le(output, static_cast<std::uint32_t>(first.sample_rate));
    write_u32_le(output, byte_rate);
    write_u16_le(output, block_align);
    write_u16_le(output, bits_per_sample);
    output.write("data", 4);
    write_u32_le(output, static_cast<std::uint32_t>(data_size));
    for (const auto& frame : frames) {
        output.write(
            reinterpret_cast<const char*>(frame.data.data()),
            static_cast<std::streamsize>(frame.data.size()));
    }

    if (!output) {
        return Status::internal("WAV write failed");
    }

    return Status::ok_status();
}

Status write_ppm_file(
    const std::filesystem::path& path,
    const MediaFrame& frame) {
    if (path.empty()) {
        return Status::invalid_argument("PPM path cannot be empty");
    }
    if (frame.type != MediaStreamType::video ||
        frame.width <= 0 ||
        frame.height <= 0 ||
        frame.data.empty()) {
        return Status::invalid_argument("PPM writer requires a non-empty video frame");
    }

    const auto pixel_count = static_cast<std::size_t>(frame.width) *
        static_cast<std::size_t>(frame.height);
    const bool is_rgb24 = frame.format_name == "rgb24";
    const bool is_rgba = frame.format_name == "rgba";
    if (!is_rgb24 && !is_rgba) {
        return Status::invalid_argument("PPM writer requires rgb24 or rgba frames");
    }

    const auto expected_size = pixel_count * (is_rgb24 ? 3u : 4u);
    if (frame.data.size() < expected_size) {
        return Status::invalid_argument("PPM frame data is smaller than expected");
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return Status::invalid_argument("PPM path cannot be opened");
    }

    output << "P6\n" << frame.width << " " << frame.height << "\n255\n";
    if (is_rgb24) {
        output.write(
            reinterpret_cast<const char*>(frame.data.data()),
            static_cast<std::streamsize>(expected_size));
    } else {
        for (std::size_t pixel = 0; pixel < pixel_count; ++pixel) {
            const auto offset = pixel * 4u;
            output.put(static_cast<char>(frame.data[offset]));
            output.put(static_cast<char>(frame.data[offset + 1]));
            output.put(static_cast<char>(frame.data[offset + 2]));
        }
    }

    if (!output) {
        return Status::internal("PPM write failed");
    }

    return Status::ok_status();
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
    return open(path, MediaDecodeOptions{});
}

Result<MediaDecoder> MediaDecoder::open(
    const std::filesystem::path& path,
    const MediaDecodeOptions& options) {
    const std::string operation = "Media decoder open";
    diagnostic_log(log::Level::debug, operation + " path=" + path_string(path));

    const auto validation = validate_media_path(path, operation);
    if (!validation.ok()) {
        return validation;
    }

#if !defined(NEXUS_MEDIA_WITH_FFMPEG)
    static_cast<void>(options);
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

    const auto media_type = to_av_media_type(options.stream_type);
    if (media_type == AVMEDIA_TYPE_UNKNOWN || media_type == AVMEDIA_TYPE_SUBTITLE) {
        status = Status::invalid_argument("media decoder stream type is not supported");
        diagnostic_log(log::Level::warn, operation + " failed: " + status.message());
        return status;
    }

    const auto stream_index = av_find_best_stream(
        format_context.get(),
        media_type,
        -1,
        -1,
        nullptr,
        0);
    if (stream_index < 0) {
        status = Status::not_found("media decoder stream not found");
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
        stream_index,
        options.stream_type));
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
        result.type = storage_->stream_type();
        result.pts = frame->pts == AV_NOPTS_VALUE ? 0 : frame->pts;

        if (result.type == MediaStreamType::audio) {
            const auto sample_format = static_cast<AVSampleFormat>(frame->format);
            const char* sample_format_name = av_get_sample_fmt_name(sample_format);
            result.format_name = sample_format_name == nullptr ? "" : sample_format_name;
            result.bytes_per_sample = av_get_bytes_per_sample(sample_format);
            result.planar = av_sample_fmt_is_planar(sample_format) != 0;
            result.sample_rate = frame->sample_rate;
            result.channels = frame_channel_count(*frame);
        } else if (result.type == MediaStreamType::video) {
            const auto pixel_format = static_cast<AVPixelFormat>(frame->format);
            const char* pixel_format_name = av_get_pix_fmt_name(pixel_format);
            result.format_name = pixel_format_name == nullptr ? "" : pixel_format_name;
            result.width = frame->width;
            result.height = frame->height;
        }

        if (result.type == MediaStreamType::audio &&
            result.bytes_per_sample > 0 &&
            frame->nb_samples > 0) {
            if (result.planar) {
                const auto plane_size = frame->nb_samples * result.bytes_per_sample;
                result.data.reserve(static_cast<std::size_t>(plane_size * result.channels));
                for (int channel = 0; channel < result.channels; ++channel) {
                    if (frame->data[channel] != nullptr) {
                        result.data.insert(
                            result.data.end(),
                            frame->data[channel],
                            frame->data[channel] + plane_size);
                    }
                }
            } else if (frame->data[0] != nullptr) {
                const auto byte_count =
                    frame->nb_samples * result.channels * result.bytes_per_sample;
                result.data.assign(frame->data[0], frame->data[0] + byte_count);
            }
        } else if (result.type == MediaStreamType::video && result.width > 0 && result.height > 0) {
            const auto pixel_format = static_cast<AVPixelFormat>(frame->format);
            const auto buffer_size =
                av_image_get_buffer_size(pixel_format, result.width, result.height, 1);
            if (buffer_size < 0) {
                return Status::invalid_argument(
                    "FFmpeg image buffer size failed: " + ffmpeg_error(buffer_size));
            }
            result.data.resize(static_cast<std::size_t>(buffer_size));
            const auto copy_result = av_image_copy_to_buffer(
                result.data.data(),
                buffer_size,
                frame->data,
                frame->linesize,
                pixel_format,
                result.width,
                result.height,
                1);
            if (copy_result < 0) {
                return Status::invalid_argument(
                    "FFmpeg image copy failed: " + ffmpeg_error(copy_result));
            }
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
