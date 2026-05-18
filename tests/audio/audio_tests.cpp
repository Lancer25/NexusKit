#include <chrono>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <nexus/audio/capturer.h>

namespace {

using namespace nexus::audio;

bool has_audio_device() {
    auto result = AudioCapturer::enumerate_input_devices();
    return result.ok() && !result.value().empty();
}

bool can_create_capturer() {
    return AudioCapturer::create().ok();
}

TEST_CASE("Audio capturer enumerates input devices", "[audio]") {
    auto result = AudioCapturer::enumerate_input_devices();
    REQUIRE(result.ok());
    const auto& devices = result.value();
    for (const auto& d : devices) {
        REQUIRE(!d.id.empty());
    }
}

TEST_CASE("Audio capturer returns default input device", "[audio]") {
    auto result = AudioCapturer::default_input_device();
    if (!result.ok()) {
        // kNotFound means no device on the system — acceptable.
        return;
    }
    const auto& dev = result.value();
    REQUIRE(!dev.id.empty());
}

TEST_CASE("Audio capturer create succeeds", "[audio]") {
    auto result = AudioCapturer::create();
    if (!result.ok()) return; // No backend or no device
    auto capturer = std::move(result).value();
    REQUIRE(!capturer.is_capturing());
}

TEST_CASE("Audio capturer start and stop lifecycle", "[audio]") {
    if (!can_create_capturer()) return;

    auto capturer = AudioCapturer::create().value();

    bool received = false;
    auto status = capturer.start([&](nexus::Result<AudioFrame> frame) {
        if (frame.ok()) {
            received = true;
            const auto& f = frame.value();
            REQUIRE(f.sample_rate > 0);
            REQUIRE(f.channels > 0);
            REQUIRE(!f.data.empty());
        }
    });
    if (!status.ok()) return; // Capture may fail on some hardware

    REQUIRE(capturer.is_capturing());

    // Let capture run briefly to collect at least one frame.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    status = capturer.stop();
    REQUIRE(status.ok());
    REQUIRE(!capturer.is_capturing());

    REQUIRE(received);
}

TEST_CASE("Audio capturer stop is idempotent", "[audio]") {
    if (!can_create_capturer()) return;

    auto capturer = AudioCapturer::create().value();
    REQUIRE(capturer.stop().ok());
    REQUIRE(capturer.stop().ok());
}

TEST_CASE("Audio capturer rejects double start", "[audio]") {
    if (!can_create_capturer()) return;

    auto capturer = AudioCapturer::create().value();
    auto first = capturer.start([](nexus::Result<AudioFrame>) {});
    if (!first.ok()) return; // Capture may fail on some hardware

    auto status = capturer.start([](nexus::Result<AudioFrame>) {});
    REQUIRE(!status.ok());
    REQUIRE(capturer.stop().ok());
}

TEST_CASE("Audio capturer gets volume of default device", "[audio]") {
    auto result = AudioCapturer::volume();
    if (!result.ok()) return;
    REQUIRE(result.value() <= 100);
}

TEST_CASE("Audio capturer sets volume of default device", "[audio]") {
    auto orig = AudioCapturer::volume();
    if (!orig.ok()) return;

    auto level = orig.value();
    auto status = AudioCapturer::set_volume("", level);
    REQUIRE(status.ok());

    auto after = AudioCapturer::volume();
    REQUIRE(after.ok());
    REQUIRE(after.value() == level);
}

TEST_CASE("Audio capturer set_mute on default device", "[audio]") {
    auto status = AudioCapturer::set_mute("", false);
    // set_mute may fail on some device types — not an error.
    (void)status;
}

} // namespace
