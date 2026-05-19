#include <chrono>
#include <cstring>
#include <thread>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <nexus/audio/player.h>

namespace {

using namespace nexus::audio;

bool has_output_device() {
    auto result = AudioPlayer::enumerate_output_devices();
    return result.ok() && !result.value().empty();
}

bool can_create_player() {
    return AudioPlayer::create().ok();
}

std::vector<uint8_t> make_silence(unsigned int samples,
                                   unsigned int channels) {
    return std::vector<uint8_t>(samples * channels * 2, 0);
}

std::vector<uint8_t> make_tone(unsigned int samples,
                                unsigned int channels,
                                unsigned int rate,
                                float freq_hz) {
    std::vector<uint8_t> data(samples * channels * 2, 0);
    auto* p = reinterpret_cast<std::int16_t*>(data.data());
    for (unsigned int i = 0; i < samples; ++i) {
        float t = static_cast<float>(i) / static_cast<float>(rate);
        auto val = static_cast<std::int16_t>(16000.0f * sin(2.0f * 3.14159f * freq_hz * t));
        for (unsigned int c = 0; c < channels; ++c) {
            p[i * channels + c] = val;
        }
    }
    return data;
}

} // namespace

TEST_CASE("Audio player enumerates output devices", "[audio]") {
    auto result = AudioPlayer::enumerate_output_devices();
    REQUIRE(result.ok());
    const auto& devices = result.value();
    for (const auto& d : devices) {
        REQUIRE(!d.id.empty());
    }
}

TEST_CASE("Audio player returns default output device", "[audio]") {
    auto result = AudioPlayer::default_output_device();
    if (!result.ok()) {
        return;
    }
    const auto& dev = result.value();
    REQUIRE(!dev.id.empty());
}

TEST_CASE("Audio player create succeeds", "[audio]") {
    if (!has_output_device()) return;

    auto result = AudioPlayer::create();
    if (!result.ok()) return;
    auto player = std::move(result).value();
    REQUIRE(!player.is_playing());
}

TEST_CASE("Audio player start and stop lifecycle", "[audio]") {
    if (!can_create_player()) return;

    auto player = AudioPlayer::create().value();
    REQUIRE(!player.is_playing());

    int call_count = 0;
    auto status = player.start([&]() -> nexus::Result<AudioFrame> {
        ++call_count;
        if (call_count > 10) {
            // Return empty to stop.
            return AudioFrame{};
        }
        AudioFrame frame;
        frame.sample_rate = 44100;
        frame.channels = 2;
        frame.data = make_silence(44100 / 10, 2); // 100ms of silence
        return frame;
    });

    if (!status.ok()) return;
    REQUIRE(player.is_playing());

    // Wait for playback to finish.
    for (int i = 0; i < 50 && player.is_playing(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    REQUIRE_FALSE(player.is_playing());
    REQUIRE(call_count > 0);
}

TEST_CASE("Audio player stop is idempotent", "[audio]") {
    if (!can_create_player()) return;

    auto player = AudioPlayer::create().value();
    CHECK(player.stop().ok());
    CHECK(player.stop().ok());
    CHECK_FALSE(player.is_playing());
}

TEST_CASE("Audio player double start returns error", "[audio]") {
    if (!can_create_player()) return;

    auto player = AudioPlayer::create().value();

    auto status = player.start([]() -> nexus::Result<AudioFrame> {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        return AudioFrame{};
    });

    if (!status.ok()) return;

    auto status2 = player.start([]() -> nexus::Result<AudioFrame> {
        return AudioFrame{};
    });
    REQUIRE_FALSE(status2.ok());
    CHECK(status2.code() == nexus::StatusCode::kFailedPrecondition);

    player.stop();
}

TEST_CASE("Audio player move transfers ownership", "[audio]") {
    if (!can_create_player()) return;

    auto r1 = AudioPlayer::create();
    REQUIRE(r1.ok());
    auto p1 = std::move(r1).value();
    CHECK(!p1.is_playing());

    auto p2 = std::move(p1);
    CHECK_FALSE(p1.is_playing());
}

TEST_CASE("Audio player renders tone", "[audio]") {
    if (!can_create_player()) return;

    auto player = AudioPlayer::create().value();

    int call_count = 0;
    auto status = player.start([&]() -> nexus::Result<AudioFrame> {
        ++call_count;
        if (call_count > 30) return AudioFrame{};

        AudioFrame frame;
        frame.sample_rate = 44100;
        frame.channels = 2;
        frame.data = make_tone(44100 / 10, 2, 44100, 440.0f); // 100ms tone
        return frame;
    });

    if (!status.ok()) return;
    REQUIRE(player.is_playing());

    for (int i = 0; i < 60 && player.is_playing(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    REQUIRE_FALSE(player.is_playing());
    REQUIRE(call_count > 0);
}
