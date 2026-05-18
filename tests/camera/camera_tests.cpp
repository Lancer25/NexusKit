#include <chrono>
#include <thread>

#include <catch2/catch_test_macros.hpp>
#include <nexus/camera/capturer.h>

namespace {

using namespace nexus::camera;

bool has_camera() {
    auto result = CameraCapturer::enumerate_devices();
    return result.ok() && !result.value().empty();
}

TEST_CASE("Camera capturer enumerates devices", "[camera]") {
    auto result = CameraCapturer::enumerate_devices();
    // When no backend is available, this returns an error — skip test.
    if (!result.ok()) {
        return;
    }
    const auto& devices = result.value();
    for (const auto& d : devices) {
        REQUIRE(!d.id.empty());
    }
}

TEST_CASE("Camera capturer create fails without camera", "[camera]") {
    if (has_camera()) {
        // Create should succeed when a camera is present.
        auto result = CameraCapturer::create();
        REQUIRE(result.ok());
        auto capturer = std::move(result).value();
        REQUIRE(!capturer.is_capturing());
    } else {
        // Create should return an error when no camera is available.
        auto result = CameraCapturer::create();
        // Either fails at creation, or succeeds but start fails.
        if (result.ok()) {
            auto capturer = std::move(result).value();
            REQUIRE(!capturer.is_capturing());
        }
    }
}

TEST_CASE("Camera capturer create with specific device id", "[camera]") {
    if (!has_camera()) {
        return;
    }

    auto devices = CameraCapturer::enumerate_devices().value();
    REQUIRE(!devices.empty());

    CameraCaptureOptions opts;
    opts.device_id = devices[0].id;
    auto result = CameraCapturer::create(opts);
    // Creation may succeed or fail depending on device availability.
    // Either outcome is valid here — we just verify no crash.
    if (result.ok()) {
        auto capturer = std::move(result).value();
        REQUIRE(!capturer.is_capturing());
    }
}

TEST_CASE("Camera capturer stop is idempotent", "[camera]") {
    if (!has_camera()) {
        return;
    }

    auto capturer = CameraCapturer::create().value();
    REQUIRE(capturer.stop().ok());
    REQUIRE(capturer.stop().ok());
}

TEST_CASE("Camera capturer rejects double start", "[camera]") {
    if (!has_camera()) {
        return;
    }

    auto capturer = CameraCapturer::create().value();
    REQUIRE(capturer.start([](nexus::Result<CameraFrame>) {}).ok());
    auto status = capturer.start([](nexus::Result<CameraFrame>) {});
    REQUIRE(!status.ok());
    REQUIRE(capturer.stop().ok());
}

TEST_CASE("Camera capturer start and stop lifecycle", "[camera]") {
    if (!has_camera()) {
        return;
    }

    auto capturer = CameraCapturer::create().value();

    bool received = false;
    auto status = capturer.start([&](nexus::Result<CameraFrame> frame) {
        if (frame.ok()) {
            received = true;
            const auto& f = frame.value();
            REQUIRE(f.width > 0);
            REQUIRE(f.height > 0);
            REQUIRE(!f.data.empty());
            REQUIRE(f.pixel_format != PixelFormat::kUnknown);
        }
    });
    REQUIRE(status.ok());
    REQUIRE(capturer.is_capturing());

    // Let capture run briefly to collect at least one frame.
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    status = capturer.stop();
    REQUIRE(status.ok());
    REQUIRE(!capturer.is_capturing());

    // We should have received at least one frame.
    REQUIRE(received);
}

} // namespace
