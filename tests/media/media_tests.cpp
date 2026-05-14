#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>

#include <nexus/media/media.h>

namespace {

std::filesystem::path test_media_path(const std::string& name) {
    auto path = std::filesystem::temp_directory_path() / "nexus_media_tests" / name;
    std::filesystem::create_directories(path.parent_path());
    std::filesystem::remove(path);
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
