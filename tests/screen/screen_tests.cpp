#include <catch2/catch_test_macros.hpp>

#include <nexus/screen/screen.h>

TEST_CASE("Screen capturer reports backend availability") {
    const auto backend = nexus::screen::screen_backend_info();

    CHECK(backend.name == "screen");
#if defined(_WIN32)
    CHECK(backend.available);
#else
    CHECK_FALSE(backend.available);
#endif
    CHECK_FALSE(backend.description.empty());
}

TEST_CASE("Screen capturer captures primary display when backend is available") {
    const auto backend = nexus::screen::screen_backend_info();
    auto capturer = nexus::screen::ScreenCapturer::create();

    REQUIRE(capturer.ok());

    if (!backend.available || !capturer.value().is_available()) {
        const auto frame = capturer.value().capture_primary();
        REQUIRE_FALSE(frame.ok());
        CHECK(frame.status().code() == nexus::StatusCode::kFailedPrecondition);
        return;
    }

    const auto frame = capturer.value().capture_primary();
    REQUIRE(frame.ok());
    CHECK(frame.value().width > 0);
    CHECK(frame.value().height > 0);
    CHECK(frame.value().pixel_format == nexus::screen::ScreenPixelFormat::bgra);
    CHECK(frame.value().data.size() == static_cast<std::size_t>(frame.value().width) *
                                           static_cast<std::size_t>(frame.value().height) * 4u);
}
