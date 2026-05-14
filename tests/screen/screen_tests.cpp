#include <catch2/catch_test_macros.hpp>

#include <nexus/screen/screen.h>

TEST_CASE("Screen capturer reports backend availability") {
    const auto backend = nexus::screen::screen_backend_info();

    CHECK(backend.name == "screen");
    CHECK_FALSE(backend.available);
    CHECK_FALSE(backend.description.empty());
}

TEST_CASE("Screen capturer creates closed unavailable facade") {
    auto capturer = nexus::screen::ScreenCapturer::create();

    REQUIRE(capturer.ok());
    CHECK_FALSE(capturer.value().is_available());

    const auto frame = capturer.value().capture_primary();
    REQUIRE_FALSE(frame.ok());
    CHECK(frame.status().code() == nexus::StatusCode::kFailedPrecondition);
}
