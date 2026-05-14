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
    constexpr std::uint32_t sample_count = 16;
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
