#include <catch2/catch_test_macros.hpp>

#include <nexus/common/string.h>

#include "screen_cursor_overlay.h"
#include "screen_frame_crop.h"
#include "screen_x11_pixel_format.h"

#include <filesystem>
#include <fstream>
#include <unordered_set>
#include <iterator>
#include <string>
#include <vector>

#include <nexus/screen/screen.h>

namespace {

std::filesystem::path test_screen_path(const std::string& name) {
    auto path = std::filesystem::temp_directory_path() / "nexus_screen_tests" / name;
    std::filesystem::create_directories(path.parent_path());
    std::filesystem::remove(path);
    return path;
}

std::vector<char> read_binary_file(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

} // namespace

TEST_CASE("Screen PPM writer writes BGRA frames as RGB PPM") {
    nexus::screen::ScreenFrame frame;
    frame.width = 2;
    frame.height = 1;
    frame.pixel_format = nexus::screen::ScreenPixelFormat::bgra;
    frame.data = {
        30, 20, 10, 255,
        60, 50, 40, 128,
    };

    const auto path = test_screen_path("writer-output.ppm");
    const auto status = nexus::screen::write_ppm_file(path, frame);
    REQUIRE(status.ok());

    const auto bytes = read_binary_file(path);
    const std::vector<char> expected = {
        'P', '6', '\n', '2', ' ', '1', '\n', '2', '5', '5', '\n',
        10, 20, 30,
        40, 50, 60,
    };
    CHECK(bytes == expected);
}

TEST_CASE("Screen pixel format helper returns canonical names") {
    CHECK(nexus::screen::screen_pixel_format_name(nexus::screen::ScreenPixelFormat::bgra) ==
          "bgra");
    CHECK(nexus::screen::screen_pixel_format_name(nexus::screen::ScreenPixelFormat::rgba) ==
          "rgba");
    CHECK(nexus::screen::screen_pixel_format_name(nexus::screen::ScreenPixelFormat::unknown)
              .empty());
}

TEST_CASE("X11 pixel format converts 24-bit truecolor pixels to BGRA") {
    const nexus::screen::detail::X11PixelFormat format{
        0x00ff0000ul,
        0x0000ff00ul,
        0x000000fful,
    };

    std::uint8_t bgra[4]{};
    nexus::screen::detail::x11_pixel_to_bgra(0x00123456ul, format, bgra);

    CHECK(bgra[0] == 0x56);
    CHECK(bgra[1] == 0x34);
    CHECK(bgra[2] == 0x12);
    CHECK(bgra[3] == 0xff);
}

TEST_CASE("X11 pixel format expands 16-bit 565 pixels to BGRA") {
    const nexus::screen::detail::X11PixelFormat format{
        0xf800ul,
        0x07e0ul,
        0x001ful,
    };

    std::uint8_t bgra[4]{};
    nexus::screen::detail::x11_pixel_to_bgra(0xfffful, format, bgra);

    CHECK(bgra[0] == 255);
    CHECK(bgra[1] == 255);
    CHECK(bgra[2] == 255);
    CHECK(bgra[3] == 255);
}

TEST_CASE("Cursor overlay alpha blends BGRA pixels") {
    nexus::screen::ScreenFrame frame;
    frame.width = 2;
    frame.height = 1;
    frame.pixel_format = nexus::screen::ScreenPixelFormat::bgra;
    frame.data = {10, 20, 30, 255, 0, 0, 0, 255};

    nexus::screen::detail::CursorOverlayImage cursor;
    cursor.width = 1;
    cursor.height = 1;
    cursor.x = 1;
    cursor.y = 0;
    cursor.pixels_bgra = {0, 0, 255, 128};

    nexus::screen::detail::overlay_cursor_bgra(frame, cursor);

    CHECK(frame.data[0] == 10);
    CHECK(frame.data[1] == 20);
    CHECK(frame.data[2] == 30);
    CHECK(frame.data[3] == 255);
    CHECK(frame.data[4] == 0);
    CHECK(frame.data[5] == 0);
    CHECK(frame.data[6] == 128);
    CHECK(frame.data[7] == 255);
}

TEST_CASE("Cursor overlay clips and skips transparent pixels") {
    nexus::screen::ScreenFrame frame;
    frame.width = 2;
    frame.height = 2;
    frame.pixel_format = nexus::screen::ScreenPixelFormat::bgra;
    frame.data = {
        1, 2, 3, 255, 4, 5, 6, 255,
        7, 8, 9, 255, 10, 11, 12, 255,
    };

    nexus::screen::detail::CursorOverlayImage cursor;
    cursor.width = 2;
    cursor.height = 2;
    cursor.x = -1;
    cursor.y = 0;
    cursor.pixels_bgra = {
        100, 101, 102, 255, 200, 201, 202, 0,
        50, 51, 52, 255, 60, 61, 62, 255,
    };

    nexus::screen::detail::overlay_cursor_bgra(frame, cursor);

    CHECK(frame.data[0] == 1);
    CHECK(frame.data[1] == 2);
    CHECK(frame.data[2] == 3);
    CHECK(frame.data[3] == 255);
    CHECK(frame.data[4] == 4);
    CHECK(frame.data[5] == 5);
    CHECK(frame.data[6] == 6);
    CHECK(frame.data[7] == 255);
    CHECK(frame.data[8] == 60);
    CHECK(frame.data[9] == 61);
    CHECK(frame.data[10] == 62);
    CHECK(frame.data[11] == 255);
    CHECK(frame.data[12] == 10);
    CHECK(frame.data[13] == 11);
    CHECK(frame.data[14] == 12);
    CHECK(frame.data[15] == 255);
}

TEST_CASE("Screen frame crop copies BGRA rows") {
    nexus::screen::ScreenFrame frame;
    frame.width = 3;
    frame.height = 2;
    frame.pixel_format = nexus::screen::ScreenPixelFormat::bgra;
    frame.timestamp_ms = 42;
    frame.data = {
        1, 2, 3, 255, 4, 5, 6, 255, 7, 8, 9, 255,
        10, 11, 12, 255, 13, 14, 15, 255, 16, 17, 18, 255,
    };

    nexus::screen::ScreenCaptureRegion region;
    region.x = 1;
    region.y = 0;
    region.width = 2;
    region.height = 2;

    const auto cropped = nexus::screen::detail::crop_frame_bgra(frame, region);
    REQUIRE(cropped.ok());
    CHECK(cropped.value().width == 2);
    CHECK(cropped.value().height == 2);
    CHECK(cropped.value().pixel_format == nexus::screen::ScreenPixelFormat::bgra);
    CHECK(cropped.value().timestamp_ms == 42);
    CHECK(cropped.value().data == std::vector<std::uint8_t>{
                                      4, 5, 6, 255, 7, 8, 9, 255,
                                      13, 14, 15, 255, 16, 17, 18, 255,
                                  });
}

TEST_CASE("Screen frame crop rejects invalid regions") {
    nexus::screen::ScreenFrame frame;
    frame.width = 2;
    frame.height = 2;
    frame.pixel_format = nexus::screen::ScreenPixelFormat::bgra;
    frame.data.assign(16, 0);

    nexus::screen::ScreenCaptureRegion region;
    region.x = 1;
    region.y = 1;
    region.width = 2;
    region.height = 1;

    const auto cropped = nexus::screen::detail::crop_frame_bgra(frame, region);
    REQUIRE_FALSE(cropped.ok());
    CHECK(cropped.status().code() == nexus::StatusCode::kInvalidArgument);
}

#if defined(_WIN32)
TEST_CASE("Windows screen strings convert wide text to UTF-8") {
    const wchar_t text[] = {0x4e2d, 0x6587, 0};

    const auto utf8 = nexus::common::wide_to_utf8(text);

    const std::string expected("\xE4\xB8\xAD\xE6\x96\x87", 6);
    CHECK(utf8 == expected);
}
#endif

TEST_CASE("Screen capturer reports backend availability") {
    const auto backend = nexus::screen::screen_backend_info();

    CHECK(backend.name == "screen");
#if defined(NEXUS_SCREEN_HAS_PLATFORM_BACKEND)
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

TEST_CASE("Screen capturer enumerates displays when backend is available") {
    const auto backend = nexus::screen::screen_backend_info();
    auto capturer = nexus::screen::ScreenCapturer::create();

    REQUIRE(capturer.ok());

    if (!backend.available || !capturer.value().is_available()) {
        const auto displays = capturer.value().displays();
        REQUIRE_FALSE(displays.ok());
        CHECK(displays.status().code() == nexus::StatusCode::kFailedPrecondition);
        return;
    }

    const auto displays = capturer.value().displays();
    REQUIRE(displays.ok());
    REQUIRE_FALSE(displays.value().empty());

    bool found_primary = false;
    for (const auto& display : displays.value()) {
        CHECK_FALSE(display.id.empty());
        CHECK(display.width > 0);
        CHECK(display.height > 0);
        if (display.primary) {
            found_primary = true;
        }
    }
    CHECK(found_primary);
}

TEST_CASE("Screen capturer captures selected display when backend is available") {
    const auto backend = nexus::screen::screen_backend_info();
    auto capturer = nexus::screen::ScreenCapturer::create();

    REQUIRE(capturer.ok());

    const auto displays = capturer.value().displays();
    if (!backend.available || !capturer.value().is_available()) {
        REQUIRE_FALSE(displays.ok());
        CHECK(displays.status().code() == nexus::StatusCode::kFailedPrecondition);
        return;
    }

    REQUIRE(displays.ok());
    REQUIRE_FALSE(displays.value().empty());

    const auto& display = displays.value().front();
    const auto frame = capturer.value().capture_display(display.id);
    REQUIRE(frame.ok());
    CHECK(frame.value().width == display.width);
    CHECK(frame.value().height == display.height);
    CHECK(frame.value().pixel_format == nexus::screen::ScreenPixelFormat::bgra);
    CHECK(frame.value().data.size() == static_cast<std::size_t>(frame.value().width) *
                                           static_cast<std::size_t>(frame.value().height) * 4u);
}

TEST_CASE("Screen capturer rejects invalid capture regions") {
    auto capturer = nexus::screen::ScreenCapturer::create();

    REQUIRE(capturer.ok());

    nexus::screen::ScreenCaptureRegion region;
    region.width = 0;
    region.height = 32;

    const auto frame = capturer.value().capture_region(region);
    REQUIRE_FALSE(frame.ok());
    CHECK(frame.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("Screen capturer captures a display region when backend is available") {
    const auto backend = nexus::screen::screen_backend_info();
    auto capturer = nexus::screen::ScreenCapturer::create();

    REQUIRE(capturer.ok());

    const auto displays = capturer.value().displays();
    if (!backend.available || !capturer.value().is_available()) {
        REQUIRE_FALSE(displays.ok());
        CHECK(displays.status().code() == nexus::StatusCode::kFailedPrecondition);
        return;
    }

    REQUIRE(displays.ok());
    REQUIRE_FALSE(displays.value().empty());

    const auto& display = displays.value().front();
    nexus::screen::ScreenCaptureRegion region;
    region.display_id = display.id;
    region.width = display.width < 64 ? display.width : 64;
    region.height = display.height < 64 ? display.height : 64;

    const auto frame = capturer.value().capture_region(region);
    REQUIRE(frame.ok());
    CHECK(frame.value().width == region.width);
    CHECK(frame.value().height == region.height);
    CHECK(frame.value().pixel_format == nexus::screen::ScreenPixelFormat::bgra);
    CHECK(frame.value().data.size() == static_cast<std::size_t>(region.width) *
                                           static_cast<std::size_t>(region.height) * 4u);
}

TEST_CASE("Screen capturer enumerates capturable windows when supported") {
    const auto backend = nexus::screen::screen_backend_info();
    auto capturer = nexus::screen::ScreenCapturer::create();

    REQUIRE(capturer.ok());

    const auto windows = capturer.value().windows();
    if (!backend.available || !capturer.value().is_available()) {
        REQUIRE_FALSE(windows.ok());
        CHECK(windows.status().code() == nexus::StatusCode::kFailedPrecondition);
        return;
    }

#if defined(_WIN32)
    REQUIRE(windows.ok());
    int active_count = 0;
    for (const auto& window : windows.value()) {
        CHECK_FALSE(window.id.empty());
        CHECK_FALSE(window.display_id.empty());
        CHECK(window.process_id != 0);
        CHECK_FALSE(window.title.empty());
        CHECK(window.width > 0);
        CHECK(window.height > 0);
        if (window.active) {
            ++active_count;
        }
    }
    CHECK(active_count <= 1);
#else
    if (!windows.ok()) {
        CHECK(windows.status().code() == nexus::StatusCode::kFailedPrecondition);
        return;
    }
    for (const auto& window : windows.value()) {
        CHECK_FALSE(window.id.empty());
        CHECK_FALSE(window.display_id.empty());
        CHECK(window.process_id != 0);
        CHECK_FALSE(window.title.empty());
        CHECK(window.width > 0);
        CHECK(window.height > 0);
    }
#endif
}

TEST_CASE("Screen capturer captures a visible window when supported") {
    const auto backend = nexus::screen::screen_backend_info();
    auto capturer = nexus::screen::ScreenCapturer::create();

    REQUIRE(capturer.ok());

    const auto windows = capturer.value().windows();
    if (!backend.available || !capturer.value().is_available()) {
        REQUIRE_FALSE(windows.ok());
        CHECK(windows.status().code() == nexus::StatusCode::kFailedPrecondition);
        return;
    }

#if defined(_WIN32)
    REQUIRE(windows.ok());
    if (windows.value().empty()) {
        return;
    }

    const auto frame = capturer.value().capture_window(windows.value().front().id);
    REQUIRE(frame.ok());
    CHECK(frame.value().width == windows.value().front().width);
    CHECK(frame.value().height == windows.value().front().height);
    CHECK(frame.value().pixel_format == nexus::screen::ScreenPixelFormat::bgra);
    CHECK(frame.value().data.size() == static_cast<std::size_t>(frame.value().width) *
                                           static_cast<std::size_t>(frame.value().height) * 4u);
#else
    if (!windows.ok()) {
        CHECK(windows.status().code() == nexus::StatusCode::kFailedPrecondition);
        return;
    }
    if (windows.value().empty()) {
        return;
    }

    const auto frame = capturer.value().capture_window(windows.value().front().id);
    REQUIRE(frame.ok());
    CHECK(frame.value().width == windows.value().front().width);
    CHECK(frame.value().height == windows.value().front().height);
    CHECK(frame.value().pixel_format == nexus::screen::ScreenPixelFormat::bgra);
    CHECK(frame.value().data.size() == static_cast<std::size_t>(frame.value().width) *
                                           static_cast<std::size_t>(frame.value().height) * 4u);
#endif
}

TEST_CASE("Screen capturer captures primary display with cursor option when backend is available") {
    const auto backend = nexus::screen::screen_backend_info();
    nexus::screen::ScreenCaptureOptions options;
    options.include_cursor = true;
    auto capturer = nexus::screen::ScreenCapturer::create(options);

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

TEST_CASE("Screen capturer X11 window metadata uses expected id formats") {
    const auto backend = nexus::screen::screen_backend_info();
    auto capturer = nexus::screen::ScreenCapturer::create();

    REQUIRE(capturer.ok());

    if (!backend.available || !capturer.value().is_available()) {
        return;
    }

    const auto windows = capturer.value().windows();
#if defined(_WIN32)
    // Windows window ids start with win32-hwnd-
    if (windows.ok()) {
        for (const auto& window : windows.value()) {
            CHECK(window.id.substr(0, 11) == "win32-hwnd-");
        }
    }
#else
    if (!windows.ok()) {
        return;
    }
    for (const auto& window : windows.value()) {
        CHECK(window.id.substr(0, 12) == "x11-window-");
        CHECK((window.display_id == "x11-default" ||
               window.display_id.substr(0, 7) == "xrandr-"));
    }
#endif
}

TEST_CASE("Screen capturer maps visible windows to valid displays") {
    auto capturer = nexus::screen::ScreenCapturer::create();
    REQUIRE(capturer.ok());

    const auto backend = nexus::screen::screen_backend_info();
    if (!backend.available || !capturer.value().is_available()) {
        return;
    }

    const auto displays = capturer.value().displays();
    if (!displays.ok()) {
        CHECK(displays.status().code() == nexus::StatusCode::kFailedPrecondition);
        return;
    }

    const auto windows = capturer.value().windows();
    if (!windows.ok()) {
        CHECK(windows.status().code() == nexus::StatusCode::kFailedPrecondition);
        return;
    }

    // Build set of valid display IDs for cross-referencing window mappings.
    std::unordered_set<std::string> display_ids;
    for (const auto& d : displays.value()) {
        CHECK_FALSE(d.id.empty());
        display_ids.insert(d.id);
    }
    // Display IDs must be unique.
    CHECK(display_ids.size() == displays.value().size());

    // Every window must reference a known display.
    for (const auto& w : windows.value()) {
        CHECK_FALSE(w.display_id.empty());
        CHECK((display_ids.count(w.display_id) == 1));
    }
}
