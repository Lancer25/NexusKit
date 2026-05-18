#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

#include <nexus/common/binary.h>
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
    nexus::common::append_le32(data, 36 + data_size);
    data.insert(data.end(), {'W', 'A', 'V', 'E'});
    data.insert(data.end(), {'f', 'm', 't', ' '});
    nexus::common::append_le32(data, 16);
    nexus::common::append_le16(data, 1);
    nexus::common::append_le16(data, channels);
    nexus::common::append_le32(data, sample_rate);
    nexus::common::append_le32(data, byte_rate);
    nexus::common::append_le16(data, block_align);
    nexus::common::append_le16(data, bits_per_sample);
    data.insert(data.end(), {'d', 'a', 't', 'a'});
    nexus::common::append_le32(data, data_size);
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

TEST_CASE("Media sample and pixel format helpers return canonical names") {
    CHECK(nexus::media::audio_sample_format_name(nexus::media::AudioSampleFormat::s16) == "s16");
    CHECK(nexus::media::audio_sample_format_name(nexus::media::AudioSampleFormat::unknown).empty());
    CHECK(nexus::media::video_pixel_format_name(nexus::media::VideoPixelFormat::rgb24) == "rgb24");
    CHECK(nexus::media::video_pixel_format_name(nexus::media::VideoPixelFormat::bgra) == "bgra");
    CHECK(nexus::media::video_pixel_format_name(nexus::media::VideoPixelFormat::unknown).empty());
}

TEST_CASE("Media stream type helper returns canonical names") {
    CHECK(nexus::media::media_stream_type_name(nexus::media::MediaStreamType::video) == "video");
    CHECK(nexus::media::media_stream_type_name(nexus::media::MediaStreamType::audio) == "audio");
    CHECK(nexus::media::media_stream_type_name(nexus::media::MediaStreamType::subtitle) ==
          "subtitle");
    CHECK(nexus::media::media_stream_type_name(nexus::media::MediaStreamType::unknown).empty());
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

TEST_CASE("Media decoder opens with explicit stream index when FFmpeg backend is available") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_pcm_wav_fixture("explicit-index.wav");

    nexus::media::MediaDecodeOptions options;
    options.stream_type = nexus::media::MediaStreamType::audio;
    options.stream_index = 0;

    auto decoder = nexus::media::MediaDecoder::open(path, options);
    REQUIRE(decoder.ok());

    const auto frame = decoder.value().read_frame();
    REQUIRE(frame.ok());
    CHECK(frame.value().type == nexus::media::MediaStreamType::audio);
    CHECK(frame.value().stream_index == 0);
}

TEST_CASE("Media decoder rejects invalid stream index") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_pcm_wav_fixture("bad-index.wav");

    nexus::media::MediaDecodeOptions options;
    options.stream_type = nexus::media::MediaStreamType::audio;
    options.stream_index = 99;

    const auto decoder = nexus::media::MediaDecoder::open(path, options);
    REQUIRE_FALSE(decoder.ok());
    CHECK(decoder.status().code() == nexus::StatusCode::kNotFound);
}

TEST_CASE("Media decoder returns kNotFound at end of stream") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = write_pcm_wav_fixture("eof-check.wav");

    auto decoder = nexus::media::MediaDecoder::open(path);
    REQUIRE(decoder.ok());

    // Drain all frames until EOF.
    int frame_count = 0;
    while (true) {
        const auto frame = decoder.value().read_frame();
        if (!frame.ok()) {
            CHECK(frame.status().code() == nexus::StatusCode::kNotFound);
            break;
        }
        ++frame_count;
    }
    CHECK(frame_count > 0);

    // EOF is idempotent; repeated reads also return kNotFound.
    const auto again = decoder.value().read_frame();
    REQUIRE_FALSE(again.ok());
    CHECK(again.status().code() == nexus::StatusCode::kNotFound);
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

TEST_CASE("Media PPM writer writes BGRA video frames as RGB") {
    nexus::media::MediaFrame frame;
    frame.type = nexus::media::MediaStreamType::video;
    frame.width = 2;
    frame.height = 1;
    frame.format_name = nexus::media::video_pixel_format_name(nexus::media::VideoPixelFormat::bgra);
    frame.data = {
        30, 20, 10, 255,
        60, 50, 40, 128,
    };

    const auto output_path = test_media_path("writer-bgra-output.ppm");
    const auto status = nexus::media::write_ppm_file(output_path, frame);
    REQUIRE(status.ok());

    const std::string expected =
        std::string("P6\n2 1\n255\n", 11) +
        std::string({10, 20, 30, 40, 50, 60});
    CHECK(read_file(output_path) == expected);
}

TEST_CASE("Media PPM writer writes BGR24 video frames as RGB") {
    nexus::media::MediaFrame frame;
    frame.type = nexus::media::MediaStreamType::video;
    frame.width = 2;
    frame.height = 1;
    frame.format_name = nexus::media::video_pixel_format_name(nexus::media::VideoPixelFormat::bgr24);
    frame.data = {
        3, 2, 1,
        6, 5, 4,
    };

    const auto output_path = test_media_path("writer-bgr24-output.ppm");
    const auto status = nexus::media::write_ppm_file(output_path, frame);
    REQUIRE(status.ok());

    const std::string expected =
        std::string("P6\n2 1\n255\n", 11) +
        std::string({1, 2, 3, 4, 5, 6});
    CHECK(read_file(output_path) == expected);
}

namespace {

nexus::media::MediaFrame make_silent_audio_frame(
    int sample_rate, int channels, int samples,
    const std::string& format_name = "s16",
    int bytes_per_sample = 2,
    bool planar = false) {
    nexus::media::MediaFrame frame;
    frame.type = nexus::media::MediaStreamType::audio;
    frame.format_name = format_name;
    frame.bytes_per_sample = bytes_per_sample;
    frame.planar = planar;
    frame.sample_rate = sample_rate;
    frame.channels = channels;
    frame.data.resize(
        static_cast<std::size_t>(samples * channels * bytes_per_sample), 0);
    return frame;
}

nexus::media::MediaFrame make_test_video_frame(
    int width, int height,
    const std::string& format_name = "yuv420p") {
    nexus::media::MediaFrame frame;
    frame.type = nexus::media::MediaStreamType::video;
    frame.format_name = format_name;
    frame.width = width;
    frame.height = height;
    std::size_t y_size = static_cast<std::size_t>(width * height);
    std::size_t uv_size = static_cast<std::size_t>((width / 2) * (height / 2));
    frame.data.resize(y_size + 2 * uv_size, 0);
    std::memset(frame.data.data(), 128, y_size);
    std::memset(frame.data.data() + y_size, 128, uv_size);
    std::memset(frame.data.data() + y_size + uv_size, 128, uv_size);
    return frame;
}

} // namespace

TEST_CASE("Media encoder encodes fltp audio to AAC") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    nexus::media::MediaEncodeConfig config;
    config.codec_name = "aac";
    config.sample_rate = 44100;
    config.channels = 2;
    config.bit_rate = 64000;
    config.sample_format = nexus::media::AudioSampleFormat::fltp;

    auto encoder = nexus::media::MediaEncoder::open(config);
    REQUIRE(encoder.ok());

    // Send frames, draining packets as needed until at least one packet is
    // produced, then flush and drain remaining.
    bool got_packet = false;
    for (int i = 0; i < 10 && !got_packet; ++i) {
        auto frame = make_silent_audio_frame(44100, 2, 1024, "fltp", 4, true);
        auto send_result = encoder.value().send_frame(frame);
        if (!send_result.ok() &&
            send_result.code() == nexus::StatusCode::kResourceExhausted) {
            // Drain output before sending more.
            auto pkt = encoder.value().receive_packet();
            if (pkt.ok()) {
                got_packet = true;
                CHECK_FALSE(pkt.value().data.empty());
            }
            --i; // retry send
            continue;
        }
        REQUIRE(send_result.ok());

        auto pkt = encoder.value().receive_packet();
        if (pkt.ok()) {
            got_packet = true;
            CHECK_FALSE(pkt.value().data.empty());
        }
    }
    REQUIRE(encoder.value().flush().ok());
    // Drain flushed packets.
    while (true) {
        auto pkt = encoder.value().receive_packet();
        if (!pkt.ok()) break;
        got_packet = true;
    }
    CHECK(got_packet);
}

TEST_CASE("Media encoder encodes s16 audio to PCM") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    nexus::media::MediaEncodeConfig config;
    config.codec_name = "pcm_s16le";
    config.sample_rate = 8000;
    config.channels = 1;

    auto encoder = nexus::media::MediaEncoder::open(config);
    REQUIRE(encoder.ok());

    auto frame = make_silent_audio_frame(8000, 1, 512);
    REQUIRE(encoder.value().send_frame(frame).ok());

    auto packet = encoder.value().receive_packet();
    REQUIRE(packet.ok());
    CHECK_FALSE(packet.value().data.empty());
}

TEST_CASE("Media encoder assigns default audio timestamps by sample count") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    nexus::media::MediaEncodeConfig config;
    config.codec_name = "pcm_s16le";
    config.sample_rate = 8000;
    config.channels = 1;

    auto encoder = nexus::media::MediaEncoder::open(config);
    REQUIRE(encoder.ok());

    std::vector<nexus::media::MediaPacket> packets;
    for (int i = 0; i < 3; ++i) {
        auto frame = make_silent_audio_frame(8000, 1, 512);
        REQUIRE(encoder.value().send_frame(frame).ok());

        auto packet = encoder.value().receive_packet();
        REQUIRE(packet.ok());
        packets.push_back(packet.value());
    }

    REQUIRE(packets.size() == 3);
    CHECK(packets[0].pts == 0);
    CHECK(packets[1].pts == 512);
    CHECK(packets[2].pts == 1024);
}

TEST_CASE("Media encoder rejects empty codec name") {
    nexus::media::MediaEncodeConfig config;
    config.codec_name = "";
    config.sample_rate = 44100;
    config.channels = 2;

    const auto encoder = nexus::media::MediaEncoder::open(config);
    REQUIRE_FALSE(encoder.ok());
    const auto code = encoder.status().code();
    CHECK((code == nexus::StatusCode::kInvalidArgument ||
           code == nexus::StatusCode::kFailedPrecondition));
}

TEST_CASE("Media encoder closed operations return error") {
    nexus::media::MediaEncoder encoder;
    CHECK_FALSE(encoder.is_open());

    nexus::media::MediaFrame frame;
    frame.type = nexus::media::MediaStreamType::audio;
    CHECK_FALSE(encoder.send_frame(frame).ok());
    CHECK_FALSE(encoder.receive_packet().ok());
    CHECK_FALSE(encoder.flush().ok());
    CHECK(encoder.close().ok());
}

TEST_CASE("Media encoder move transfers ownership") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    nexus::media::MediaEncodeConfig config;
    config.codec_name = "aac";
    config.sample_rate = 44100;
    config.channels = 2;
    config.sample_format = nexus::media::AudioSampleFormat::fltp;

    auto result = nexus::media::MediaEncoder::open(config);
    REQUIRE(result.ok());
    auto e1 = std::move(result).value();
    CHECK(e1.is_open());

    auto e2 = std::move(e1);
    CHECK_FALSE(e1.is_open());
    CHECK(e2.is_open());

    CHECK(e2.close().ok());
}

TEST_CASE("Media encoder flush drains remaining packets") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    nexus::media::MediaEncodeConfig config;
    config.codec_name = "aac";
    config.sample_rate = 44100;
    config.channels = 2;
    config.sample_format = nexus::media::AudioSampleFormat::fltp;

    auto encoder = nexus::media::MediaEncoder::open(config);
    REQUIRE(encoder.ok());

    // Send frames, draining intermittently.
    for (int i = 0; i < 4; ++i) {
        auto frame = make_silent_audio_frame(44100, 2, 1024, "fltp", 4, true);
        auto sr = encoder.value().send_frame(frame);
        if (!sr.ok() && sr.code() == nexus::StatusCode::kResourceExhausted) {
            while (true) {
                auto pkt = encoder.value().receive_packet();
                if (!pkt.ok()) break;
            }
            --i;
            continue;
        }
        REQUIRE(sr.ok());
    }

    // Drain any packets produced after last send.
    while (true) {
        auto pkt = encoder.value().receive_packet();
        if (!pkt.ok()) break;
    }

    REQUIRE(encoder.value().flush().ok());

    int flushed_count = 0;
    while (true) {
        auto pkt = encoder.value().receive_packet();
        if (!pkt.ok()) {
            CHECK(pkt.status().code() == nexus::StatusCode::kNotFound);
            break;
        }
        ++flushed_count;
    }
    // AAC encoder may or may not produce flushed packets; either is valid.
    (void)flushed_count;
}

TEST_CASE("Media encoder encodes yuv420p video to H264") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    nexus::media::MediaEncodeConfig config;
    config.codec_name = "libx264";
    config.width = 320;
    config.height = 240;
    config.bit_rate = 400000;
    config.frame_rate = 25;
    config.pixel_format = "yuv420p";

    auto encoder = nexus::media::MediaEncoder::open(config);
    REQUIRE(encoder.ok());

    bool got_packet = false;
    for (int i = 0; i < 10 && !got_packet; ++i) {
        auto frame = make_test_video_frame(320, 240, "yuv420p");
        auto sr = encoder.value().send_frame(frame);
        if (!sr.ok() && sr.code() == nexus::StatusCode::kResourceExhausted) {
            auto pkt = encoder.value().receive_packet();
            if (pkt.ok()) {
                got_packet = true;
                CHECK_FALSE(pkt.value().data.empty());
            }
            --i;
            continue;
        }
        REQUIRE(sr.ok());

        auto pkt = encoder.value().receive_packet();
        if (pkt.ok()) {
            got_packet = true;
            CHECK_FALSE(pkt.value().data.empty());
        }
    }
    REQUIRE(encoder.value().flush().ok());
    while (true) {
        auto pkt = encoder.value().receive_packet();
        if (!pkt.ok()) break;
        got_packet = true;
    }
    CHECK(got_packet);
}

TEST_CASE("Media muxer writes AAC to MP4 and probe validates") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto output_path = test_media_path("muxer-aac.mp4");

    // Encode a few silent AAC packets.
    nexus::media::MediaEncodeConfig enc_config;
    enc_config.codec_name = "aac";
    enc_config.sample_rate = 44100;
    enc_config.channels = 2;
    enc_config.sample_format = nexus::media::AudioSampleFormat::fltp;

    auto encoder = nexus::media::MediaEncoder::open(enc_config);
    REQUIRE(encoder.ok());

    // Open muxer and add the audio stream.
    auto muxer = nexus::media::MediaMuxer::open(output_path);
    REQUIRE(muxer.ok());
    auto stream_idx = muxer.value().add_stream(enc_config);
    REQUIRE(stream_idx.ok());
    CHECK(stream_idx.value() == 0);

    // Encode frames, draining packets to the muxer.
    int muxed_count = 0;
    for (int i = 0; i < 10 && muxed_count < 5; ++i) {
        auto frame = make_silent_audio_frame(44100, 2, 1024, "fltp", 4, true);
        auto sr = encoder.value().send_frame(frame);
        if (!sr.ok() && sr.code() == nexus::StatusCode::kResourceExhausted) {
            auto pkt = encoder.value().receive_packet();
            if (pkt.ok()) {
                pkt.value().stream_index = stream_idx.value();
                REQUIRE(muxer.value().write_packet(pkt.value()).ok());
                ++muxed_count;
            }
            --i;
            continue;
        }
        REQUIRE(sr.ok());

        auto pkt = encoder.value().receive_packet();
        if (pkt.ok()) {
            pkt.value().stream_index = stream_idx.value();
            REQUIRE(muxer.value().write_packet(pkt.value()).ok());
            ++muxed_count;
        }
    }

    REQUIRE(encoder.value().flush().ok());
    while (true) {
        auto packet = encoder.value().receive_packet();
        if (!packet.ok()) break;
        packet.value().stream_index = stream_idx.value();
        REQUIRE(muxer.value().write_packet(packet.value()).ok());
        ++muxed_count;
    }

    CHECK(muxed_count > 0);
    REQUIRE(muxer.value().close().ok());
    REQUIRE(encoder.value().close().ok());

    // Probe the output MP4.
    const auto probe = nexus::media::probe_media(output_path);
    REQUIRE(probe.ok());
    CHECK_FALSE(probe.value().format_name.empty());
    REQUIRE(probe.value().streams.size() == 1);
    CHECK(probe.value().streams[0].type == nexus::media::MediaStreamType::audio);
    CHECK(probe.value().streams[0].sample_rate == 44100);
    CHECK(probe.value().streams[0].channels == 2);
    CHECK(probe.value().duration_ms > 0);
}

TEST_CASE("Media muxer writes MP2 to MPEG-PS and probe validates") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto output_path = test_media_path("muxer-mp2.ps");

    nexus::media::MediaEncodeConfig enc_config;
    enc_config.codec_name = "mp2";
    enc_config.sample_rate = 44100;
    enc_config.channels = 2;
    enc_config.sample_format = nexus::media::AudioSampleFormat::s16;

    auto encoder = nexus::media::MediaEncoder::open(enc_config);
    REQUIRE(encoder.ok());

    nexus::media::MediaMuxConfig mux_config;
    mux_config.format_name = "mpeg";

    auto muxer = nexus::media::MediaMuxer::open(output_path, mux_config);
    REQUIRE(muxer.ok());
    auto stream_idx = muxer.value().add_stream(enc_config);
    REQUIRE(stream_idx.ok());

    int muxed_count = 0;
    for (int i = 0; i < 12 && muxed_count < 5; ++i) {
        auto frame = make_silent_audio_frame(44100, 2, 1152, "s16", 2, false);
        auto sr = encoder.value().send_frame(frame);
        if (!sr.ok() && sr.code() == nexus::StatusCode::kResourceExhausted) {
            auto packet = encoder.value().receive_packet();
            if (packet.ok()) {
                packet.value().stream_index = stream_idx.value();
                REQUIRE(muxer.value().write_packet(packet.value()).ok());
                ++muxed_count;
            }
            --i;
            continue;
        }
        REQUIRE(sr.ok());

        auto packet = encoder.value().receive_packet();
        if (packet.ok()) {
            packet.value().stream_index = stream_idx.value();
            REQUIRE(muxer.value().write_packet(packet.value()).ok());
            ++muxed_count;
        }
    }

    REQUIRE(encoder.value().flush().ok());
    while (true) {
        auto packet = encoder.value().receive_packet();
        if (!packet.ok()) break;
        packet.value().stream_index = stream_idx.value();
        REQUIRE(muxer.value().write_packet(packet.value()).ok());
        ++muxed_count;
    }

    CHECK(muxed_count > 0);
    REQUIRE(muxer.value().close().ok());

    const auto probe = nexus::media::probe_media(output_path);
    REQUIRE(probe.ok());
    CHECK_FALSE(probe.value().format_name.empty());
    REQUIRE(probe.value().streams.size() == 1);
    CHECK(probe.value().streams[0].type == nexus::media::MediaStreamType::audio);
}

TEST_CASE("Media muxer closed operations return errors") {
    nexus::media::MediaMuxer muxer;
    CHECK_FALSE(muxer.is_open());

    nexus::media::MediaEncodeConfig config;
    config.codec_name = "aac";
    CHECK_FALSE(muxer.add_stream(config).ok());
    CHECK_FALSE(muxer.write_packet(nexus::media::MediaPacket{}).ok());
    CHECK(muxer.close().ok());
}

TEST_CASE("Media muxer move transfers ownership") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = test_media_path("muxer-move.mp4");

    auto result = nexus::media::MediaMuxer::open(path);
    REQUIRE(result.ok());
    auto m1 = std::move(result).value();
    CHECK(m1.is_open());

    auto m2 = std::move(m1);
    CHECK_FALSE(m1.is_open());
    CHECK(m2.is_open());

    CHECK(m2.close().ok());
}

TEST_CASE("Media muxer rejects empty path") {
    const auto muxer = nexus::media::MediaMuxer::open({});
    REQUIRE_FALSE(muxer.ok());
    CHECK(muxer.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("Media muxer add_stream after write is rejected") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = test_media_path("muxer-add-after-write.mp4");

    nexus::media::MediaEncodeConfig enc_config;
    enc_config.codec_name = "aac";
    enc_config.sample_rate = 44100;
    enc_config.channels = 2;
    enc_config.sample_format = nexus::media::AudioSampleFormat::fltp;

    auto encoder = nexus::media::MediaEncoder::open(enc_config);
    REQUIRE(encoder.ok());

    auto muxer = nexus::media::MediaMuxer::open(path);
    REQUIRE(muxer.ok());

    auto stream_idx = muxer.value().add_stream(enc_config);
    REQUIRE(stream_idx.ok());

    // Send frames and drain one packet to trigger header.
    bool wrote_one = false;
    for (int i = 0; i < 8 && !wrote_one; ++i) {
        auto frame = make_silent_audio_frame(44100, 2, 1024, "fltp", 4, true);
        auto sr = encoder.value().send_frame(frame);
        if (!sr.ok() && sr.code() == nexus::StatusCode::kResourceExhausted) {
            auto packet = encoder.value().receive_packet();
            if (packet.ok()) {
                packet.value().stream_index = stream_idx.value();
                REQUIRE(muxer.value().write_packet(packet.value()).ok());
                wrote_one = true;
            }
            --i;
            continue;
        }
        REQUIRE(sr.ok());
        auto packet = encoder.value().receive_packet();
        if (packet.ok()) {
            packet.value().stream_index = stream_idx.value();
            REQUIRE(muxer.value().write_packet(packet.value()).ok());
            wrote_one = true;
        }
    }
    REQUIRE(wrote_one);

    // Adding another stream after header write should fail.
    auto second = muxer.value().add_stream(enc_config);
    REQUIRE_FALSE(second.ok());
    CHECK(second.status().code() == nexus::StatusCode::kFailedPrecondition);

    REQUIRE(muxer.value().close().ok());
}

TEST_CASE("Media muxer bad stream index is rejected") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = test_media_path("muxer-bad-stream.mp4");

    auto muxer = nexus::media::MediaMuxer::open(path);
    REQUIRE(muxer.ok());

    nexus::media::MediaEncodeConfig enc_config;
    enc_config.codec_name = "aac";
    enc_config.sample_rate = 44100;
    enc_config.channels = 2;
    enc_config.sample_format = nexus::media::AudioSampleFormat::fltp;

    auto stream_idx = muxer.value().add_stream(enc_config);
    REQUIRE(stream_idx.ok());

    nexus::media::MediaPacket bad_packet;
    bad_packet.stream_index = 99;
    bad_packet.data = {0x00, 0x01, 0x02, 0x03};

    auto write_result = muxer.value().write_packet(bad_packet);
    REQUIRE_FALSE(write_result.ok());

    REQUIRE(muxer.value().close().ok());
}

TEST_CASE("Media muxer auto-detects format from extension") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = test_media_path("muxer-autoext.m4a");

    // No format_name set; should auto-detect from .m4a extension.
    nexus::media::MediaMuxConfig config;

    auto muxer = nexus::media::MediaMuxer::open(path, config);
    REQUIRE(muxer.ok());

    nexus::media::MediaEncodeConfig enc_config;
    enc_config.codec_name = "aac";
    enc_config.sample_rate = 44100;
    enc_config.channels = 2;
    enc_config.sample_format = nexus::media::AudioSampleFormat::fltp;

    auto encoder = nexus::media::MediaEncoder::open(enc_config);
    REQUIRE(encoder.ok());

    auto stream_idx = muxer.value().add_stream(enc_config);
    REQUIRE(stream_idx.ok());

    int muxed = 0;
    for (int i = 0; i < 8 && muxed < 3; ++i) {
        auto frame = make_silent_audio_frame(44100, 2, 1024, "fltp", 4, true);
        auto sr = encoder.value().send_frame(frame);
        if (!sr.ok() && sr.code() == nexus::StatusCode::kResourceExhausted) {
            auto pkt = encoder.value().receive_packet();
            if (pkt.ok()) {
                pkt.value().stream_index = stream_idx.value();
                REQUIRE(muxer.value().write_packet(pkt.value()).ok());
                ++muxed;
            }
            --i;
            continue;
        }
        REQUIRE(sr.ok());
        auto pkt = encoder.value().receive_packet();
        if (pkt.ok()) {
            pkt.value().stream_index = stream_idx.value();
            REQUIRE(muxer.value().write_packet(pkt.value()).ok());
            ++muxed;
        }
    }

    REQUIRE(encoder.value().flush().ok());
    while (true) {
        auto pkt = encoder.value().receive_packet();
        if (!pkt.ok()) break;
        pkt.value().stream_index = stream_idx.value();
        REQUIRE(muxer.value().write_packet(pkt.value()).ok());
    }

    REQUIRE(muxer.value().close().ok());

    // Probe should recognize the format.
    const auto probe = nexus::media::probe_media(path);
    REQUIRE(probe.ok());
    CHECK_FALSE(probe.value().format_name.empty());
}

TEST_CASE("Media muxer close is idempotent") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto path = test_media_path("muxer-idempotent.mp4");

    auto muxer = nexus::media::MediaMuxer::open(path);
    REQUIRE(muxer.ok());

    CHECK(muxer.value().close().ok());
    CHECK(muxer.value().close().ok());
    CHECK(muxer.value().close().ok());
    CHECK_FALSE(muxer.value().is_open());
}

TEST_CASE("Media muxer interleaves AAC and H264 into MP4") {
    const auto backend = nexus::media::ffmpeg_backend_info();
    if (!backend.available) {
        return;
    }

    const auto output_path = test_media_path("muxer-av.mp4");

    nexus::media::MediaEncodeConfig audio_cfg;
    audio_cfg.codec_name = "aac";
    audio_cfg.sample_rate = 44100;
    audio_cfg.channels = 2;
    audio_cfg.bit_rate = 64000;
    audio_cfg.sample_format = nexus::media::AudioSampleFormat::fltp;

    nexus::media::MediaEncodeConfig video_cfg;
    video_cfg.codec_name = "libx264";
    video_cfg.width = 320;
    video_cfg.height = 240;
    video_cfg.bit_rate = 400000;
    video_cfg.frame_rate = 25;
    video_cfg.pixel_format = "yuv420p";

    auto aenc = nexus::media::MediaEncoder::open(audio_cfg);
    REQUIRE(aenc.ok());
    auto venc = nexus::media::MediaEncoder::open(video_cfg);
    REQUIRE(venc.ok());

    auto muxer = nexus::media::MediaMuxer::open(output_path);
    REQUIRE(muxer.ok());

    auto audio_idx = muxer.value().add_stream(audio_cfg);
    REQUIRE(audio_idx.ok());
    CHECK(audio_idx.value() == 0);

    auto video_idx = muxer.value().add_stream(video_cfg);
    REQUIRE(video_idx.ok());
    CHECK(video_idx.value() == 1);

    auto drain = [](nexus::media::MediaEncoder& enc, int si,
                    std::vector<nexus::media::MediaPacket>& out) {
        while (true) {
            auto pkt = enc.receive_packet();
            if (!pkt.ok()) break;
            pkt.value().stream_index = si;
            out.push_back(std::move(pkt).value());
        }
    };

    // Encode 5 video frames (~200ms at 25fps).
    std::vector<nexus::media::MediaPacket> vpkts;
    for (int i = 0; i < 5; ++i) {
        auto frame = make_test_video_frame(320, 240, "yuv420p");
        frame.pts = i;
        auto sr = venc.value().send_frame(frame);
        if (!sr.ok() && sr.code() == nexus::StatusCode::kResourceExhausted) {
            drain(venc.value(), video_idx.value(), vpkts);
            --i;
            continue;
        }
        REQUIRE(sr.ok());
        drain(venc.value(), video_idx.value(), vpkts);
    }
    REQUIRE(venc.value().flush().ok());
    drain(venc.value(), video_idx.value(), vpkts);

    // Encode audio to cover ~200ms (9 frames * 1024 samples / 44100 Hz ≈ 209ms).
    std::vector<nexus::media::MediaPacket> apkts;
    int total_samples = 0;
    for (int i = 0; i < 9; ++i) {
        auto frame = make_silent_audio_frame(44100, 2, 1024, "fltp", 4, true);
        frame.pts = total_samples;
        auto sr = aenc.value().send_frame(frame);
        if (!sr.ok() && sr.code() == nexus::StatusCode::kResourceExhausted) {
            drain(aenc.value(), audio_idx.value(), apkts);
            --i;
            continue;
        }
        REQUIRE(sr.ok());
        total_samples += 1024;
        drain(aenc.value(), audio_idx.value(), apkts);
    }
    REQUIRE(aenc.value().flush().ok());
    drain(aenc.value(), audio_idx.value(), apkts);

    REQUIRE_FALSE(apkts.empty());
    REQUIRE_FALSE(vpkts.empty());

    // Rough interleave: ~2 audio packets per video frame.
    // Audio: 1024 samples * 2 = 2048 samples ≈ 46ms. Video: 1 frame = 40ms.
    std::size_t ai = 0;
    std::size_t vi = 0;
    while (ai < apkts.size() || vi < vpkts.size()) {
        int n = 2;
        while (n-- > 0 && ai < apkts.size()) {
            REQUIRE(muxer.value().write_packet(apkts[ai++]).ok());
        }
        if (vi < vpkts.size()) {
            REQUIRE(muxer.value().write_packet(vpkts[vi++]).ok());
        }
    }

    REQUIRE(muxer.value().close().ok());
    REQUIRE(aenc.value().close().ok());
    REQUIRE(venc.value().close().ok());

    auto probe = nexus::media::probe_media(output_path);
    REQUIRE(probe.ok());
    REQUIRE(probe.value().streams.size() == 2);

    bool has_audio = false;
    bool has_video = false;
    for (const auto& s : probe.value().streams) {
        if (s.type == nexus::media::MediaStreamType::audio) has_audio = true;
        if (s.type == nexus::media::MediaStreamType::video) has_video = true;
    }
    CHECK(has_audio);
    CHECK(has_video);
}
