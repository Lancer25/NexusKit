#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include <nexus/log/logger.h>
#include <nexus/media/media.h>

namespace {

std::string read_file(const std::filesystem::path& path) {
    std::ifstream input(path);
    std::ostringstream output;
    output << input.rdbuf();
    return output.str();
}

std::filesystem::path test_media_path(const std::string& name) {
    auto path = std::filesystem::temp_directory_path() / "nexus_media_tests" / name;
    std::filesystem::create_directories(path.parent_path());
    std::filesystem::remove(path);
    return path;
}

void append_u16_le(std::vector<std::uint8_t>& data, std::uint16_t value) {
    data.push_back(static_cast<std::uint8_t>(value & 0xff));
    data.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
}

void append_u32_le(std::vector<std::uint8_t>& data, std::uint32_t value) {
    data.push_back(static_cast<std::uint8_t>(value & 0xff));
    data.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    data.push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
    data.push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
}

std::filesystem::path write_pcm_wav_fixture(const std::string& name) {
    const auto path = test_media_path(name);
    constexpr std::uint16_t channels = 1;
    constexpr std::uint32_t sample_rate = 8000;
    constexpr std::uint16_t bits_per_sample = 16;
    constexpr std::uint16_t block_align = channels * bits_per_sample / 8;
    constexpr std::uint32_t byte_rate = sample_rate * block_align;
    constexpr std::uint32_t sample_count = 1024;
    constexpr std::uint32_t data_size = sample_count * block_align;

    std::vector<std::uint8_t> data;
    data.insert(data.end(), {'R', 'I', 'F', 'F'});
    append_u32_le(data, 36 + data_size);
    data.insert(data.end(), {'W', 'A', 'V', 'E'});
    data.insert(data.end(), {'f', 'm', 't', ' '});
    append_u32_le(data, 16);
    append_u16_le(data, 1);
    append_u16_le(data, channels);
    append_u32_le(data, sample_rate);
    append_u32_le(data, byte_rate);
    append_u16_le(data, block_align);
    append_u16_le(data, bits_per_sample);
    data.insert(data.end(), {'d', 'a', 't', 'a'});
    append_u32_le(data, data_size);
    data.resize(data.size() + data_size, 0);

    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
    return path;
}

std::filesystem::path write_ppm_fixture(const std::string& name) {
    const auto path = test_media_path(name);

    std::ofstream file(path, std::ios::binary);
    file << "P6\n2 2\n255\n";
    const unsigned char pixels[] = {
        255, 0, 0,
        0, 255, 0,
        0, 0, 255,
        255, 255, 255,
    };
    file.write(reinterpret_cast<const char*>(pixels), static_cast<std::streamsize>(sizeof(pixels)));
    return path;
}

} // namespace

TEST_CASE("Media backend reports linked FFmpeg versions") {
    const auto backend = nexus::media::ffmpeg_backend_info();

    CHECK(backend.name == "ffmpeg");
    if (backend.available) {
        CHECK(backend.avutil > 0);
        CHECK(backend.avcodec > 0);
        CHECK(backend.avformat > 0);
        CHECK_FALSE(backend.configuration.empty());
    } else {
        CHECK(backend.avutil == 0);
        CHECK(backend.avcodec == 0);
        CHECK(backend.avformat == 0);
        CHECK(backend.configuration.empty());
    }
}

TEST_CASE("Media probe writes diagnostic logs when a default logger is installed") {
    const auto log_path = test_media_path("media_probe_diagnostics.log");
    const auto media_path = test_media_path("missing-diagnostics.media");

    nexus::log::LoggerOptions options;
    options.level = nexus::log::Level::debug;
    auto logger = nexus::log::create_file_logger("media_probe_diagnostics", log_path, options);
    REQUIRE(logger.ok());

    nexus::log::clear_default_logger();
    nexus::log::set_default_logger(logger.value());

    const auto probe = nexus::media::probe_media(media_path);
    REQUIRE_FALSE(probe.ok());

    nexus::log::default_logger().flush();
    nexus::log::clear_default_logger();

    const auto contents = read_file(log_path);
    REQUIRE(contents.find("Media probe path=") != std::string::npos);
    REQUIRE(contents.find("Media probe failed: media path does not exist") != std::string::npos);
}

TEST_CASE("Media probe rejects empty paths") {
    const auto probe = nexus::media::probe_media({});

    REQUIRE_FALSE(probe.ok());
    CHECK(probe.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("Media probe reports missing files") {
    const auto path = test_media_path("missing-input.media");

    const auto probe = nexus::media::probe_media(path);

    REQUIRE_FALSE(probe.ok());
    CHECK(probe.status().code() == nexus::StatusCode::kNotFound);
}

TEST_CASE("Media probe reports unavailable backend before parsing") {
    const auto path = test_media_path("placeholder.media");
    {
        std::ofstream file(path, std::ios::binary);
        file << "not a real media file";
    }

    const auto backend = nexus::media::ffmpeg_backend_info();
    const auto probe = nexus::media::probe_media(path);

    if (backend.available) {
        REQUIRE_FALSE(probe.ok());
        CHECK(probe.status().code() == nexus::StatusCode::kInvalidArgument);
    } else {
        REQUIRE_FALSE(probe.ok());
        CHECK(probe.status().code() == nexus::StatusCode::kFailedPrecondition);
    }
}

TEST_CASE("Media probe reads WAV metadata when FFmpeg backend is available") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_pcm_wav_fixture("tone.wav");

    const auto probe = nexus::media::probe_media(path);

    REQUIRE(probe.ok());
    CHECK_FALSE(probe.value().format_name.empty());
    REQUIRE(probe.value().streams.size() == 1);
    CHECK(probe.value().streams[0].type == nexus::media::MediaStreamType::audio);
    CHECK(probe.value().streams[0].sample_rate == 8000);
    CHECK(probe.value().streams[0].channels == 1);
}

TEST_CASE("Media reader rejects empty paths") {
    const auto reader = nexus::media::MediaReader::open({});

    REQUIRE_FALSE(reader.ok());
    CHECK(reader.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("Media reader reports missing files") {
    const auto path = test_media_path("missing-reader.media");

    const auto reader = nexus::media::MediaReader::open(path);

    REQUIRE_FALSE(reader.ok());
    CHECK(reader.status().code() == nexus::StatusCode::kNotFound);
}

TEST_CASE("Media reader reports unavailable backend before opening") {
    const auto path = test_media_path("reader-placeholder.media");
    {
        std::ofstream file(path, std::ios::binary);
        file << "not a real media file";
    }

    const auto backend = nexus::media::ffmpeg_backend_info();
    const auto reader = nexus::media::MediaReader::open(path);

    if (backend.available) {
        REQUIRE_FALSE(reader.ok());
        CHECK(reader.status().code() == nexus::StatusCode::kInvalidArgument);
    } else {
        REQUIRE_FALSE(reader.ok());
        CHECK(reader.status().code() == nexus::StatusCode::kFailedPrecondition);
    }
}

TEST_CASE("Media reader reads packets when FFmpeg backend is available") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_pcm_wav_fixture("reader-tone.wav");

    auto reader = nexus::media::MediaReader::open(path);
    REQUIRE(reader.ok());
    CHECK(reader.value().is_open());

    const auto packet = reader.value().read_packet();
    REQUIRE(packet.ok());
    CHECK(packet.value().stream_index == 0);
    CHECK_FALSE(packet.value().data.empty());

    CHECK(reader.value().close().ok());
    CHECK_FALSE(reader.value().is_open());
}

TEST_CASE("Media decoder rejects empty paths") {
    const auto decoder = nexus::media::MediaDecoder::open({});

    REQUIRE_FALSE(decoder.ok());
    CHECK(decoder.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("Media decoder reports missing files") {
    const auto path = test_media_path("missing-decoder.media");

    const auto decoder = nexus::media::MediaDecoder::open(path);

    REQUIRE_FALSE(decoder.ok());
    CHECK(decoder.status().code() == nexus::StatusCode::kNotFound);
}

TEST_CASE("Media decoder reports unavailable backend before opening") {
    const auto path = test_media_path("decoder-placeholder.media");
    {
        std::ofstream file(path, std::ios::binary);
        file << "not a real media file";
    }

    const auto backend = nexus::media::ffmpeg_backend_info();
    const auto decoder = nexus::media::MediaDecoder::open(path);

    if (backend.available) {
        REQUIRE_FALSE(decoder.ok());
        CHECK(decoder.status().code() == nexus::StatusCode::kInvalidArgument);
    } else {
        REQUIRE_FALSE(decoder.ok());
        CHECK(decoder.status().code() == nexus::StatusCode::kFailedPrecondition);
    }
}

TEST_CASE("Media decoder reads audio frames when FFmpeg backend is available") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_pcm_wav_fixture("decoder-tone.wav");

    auto decoder = nexus::media::MediaDecoder::open(path);
    REQUIRE(decoder.ok());
    CHECK(decoder.value().is_open());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());
    CHECK(frame.value().type == nexus::media::MediaStreamType::audio);
    CHECK(frame.value().sample_rate == 8000);
    CHECK(frame.value().channels == 1);
    CHECK_FALSE(frame.value().data.empty());

    CHECK(decoder.value().close().ok());
    CHECK_FALSE(decoder.value().is_open());
}

TEST_CASE("Media decoder accepts explicit audio options") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_pcm_wav_fixture("decoder-options-audio.wav");
    nexus::media::MediaDecodeOptions options;
    options.stream_type = nexus::media::MediaStreamType::audio;

    auto decoder = nexus::media::MediaDecoder::open(path, options);
    REQUIRE(decoder.ok());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());
    CHECK(frame.value().type == nexus::media::MediaStreamType::audio);
    CHECK(frame.value().sample_rate == 8000);
    CHECK(frame.value().channels == 1);
    CHECK(frame.value().format_name == "s16");
    CHECK(frame.value().bytes_per_sample == 2);
    CHECK_FALSE(frame.value().planar);
    CHECK_FALSE(frame.value().data.empty());
    REQUIRE(frame.value().bytes_per_sample > 0);
    REQUIRE(frame.value().channels > 0);
    CHECK(frame.value().data.size() % static_cast<std::size_t>(
        frame.value().channels * frame.value().bytes_per_sample) == 0);
}

TEST_CASE("Media decoder reads video frames when FFmpeg backend is available") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_ppm_fixture("decoder-video.ppm");
    nexus::media::MediaDecodeOptions options;
    options.stream_type = nexus::media::MediaStreamType::video;

    auto decoder = nexus::media::MediaDecoder::open(path, options);
    REQUIRE(decoder.ok());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());
    CHECK(frame.value().type == nexus::media::MediaStreamType::video);
    CHECK(frame.value().width == 2);
    CHECK(frame.value().height == 2);
    CHECK_FALSE(frame.value().format_name.empty());
    CHECK_FALSE(frame.value().data.empty());
}

TEST_CASE("Media audio converter resamples decoded audio frames when FFmpeg backend is available") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_pcm_wav_fixture("convert-audio.wav");
    auto decoder = nexus::media::MediaDecoder::open(path);
    REQUIRE(decoder.ok());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());

    nexus::media::AudioConvertOptions options;
    options.sample_rate = 16000;
    options.channels = 1;
    options.sample_format = nexus::media::AudioSampleFormat::s16;

    const auto converted = nexus::media::convert_audio_frame(frame.value(), options);
    REQUIRE(converted.ok());
    CHECK(converted.value().type == nexus::media::MediaStreamType::audio);
    CHECK(converted.value().sample_rate == 16000);
    CHECK(converted.value().channels == 1);
    CHECK(converted.value().format_name == "s16");
    CHECK(converted.value().bytes_per_sample == 2);
    CHECK_FALSE(converted.value().planar);
    CHECK_FALSE(converted.value().data.empty());
}

TEST_CASE("Media video converter converts decoded frames to RGBA when FFmpeg backend is available") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_ppm_fixture("convert-video.ppm");
    nexus::media::MediaDecodeOptions decode_options;
    decode_options.stream_type = nexus::media::MediaStreamType::video;
    auto decoder = nexus::media::MediaDecoder::open(path, decode_options);
    REQUIRE(decoder.ok());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());

    nexus::media::VideoConvertOptions options;
    options.pixel_format = nexus::media::VideoPixelFormat::rgba;

    const auto converted = nexus::media::convert_video_frame(frame.value(), options);
    REQUIRE(converted.ok());
    CHECK(converted.value().type == nexus::media::MediaStreamType::video);
    CHECK(converted.value().width == 2);
    CHECK(converted.value().height == 2);
    CHECK(converted.value().format_name == "rgba");
    CHECK(converted.value().data.size() == 2 * 2 * 4);
}

TEST_CASE("Media WAV writer writes decoded PCM audio frames") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto input_path = write_pcm_wav_fixture("writer-input.wav");
    auto decoder = nexus::media::MediaDecoder::open(input_path);
    REQUIRE(decoder.ok());
    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());

    const auto output_path = test_media_path("writer-output.wav");
    const auto status = nexus::media::write_wav_file(output_path, {frame.value()});
    REQUIRE(status.ok());

    const auto probe = nexus::media::probe_media(output_path);
    REQUIRE(probe.ok());
    REQUIRE(probe.value().streams.size() == 1);
    CHECK(probe.value().streams[0].type == nexus::media::MediaStreamType::audio);
    CHECK(probe.value().streams[0].sample_rate == frame.value().sample_rate);
    CHECK(probe.value().streams[0].channels == frame.value().channels);
}

TEST_CASE("Media PPM writer writes RGB video frames") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto input_path = write_ppm_fixture("writer-input.ppm");
    nexus::media::MediaDecodeOptions decode_options;
    decode_options.stream_type = nexus::media::MediaStreamType::video;
    auto decoder = nexus::media::MediaDecoder::open(input_path, decode_options);
    REQUIRE(decoder.ok());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());

    nexus::media::VideoConvertOptions convert_options;
    convert_options.pixel_format = nexus::media::VideoPixelFormat::rgb24;
    const auto rgb = nexus::media::convert_video_frame(frame.value(), convert_options);
    REQUIRE(rgb.ok());

    const auto output_path = test_media_path("writer-output.ppm");
    const auto status = nexus::media::write_ppm_file(output_path, rgb.value());
    REQUIRE(status.ok());

    const auto probe = nexus::media::probe_media(output_path);
    REQUIRE(probe.ok());
    REQUIRE(probe.value().streams.size() == 1);
    CHECK(probe.value().streams[0].type == nexus::media::MediaStreamType::video);
    CHECK(probe.value().streams[0].width == 2);
    CHECK(probe.value().streams[0].height == 2);
}
