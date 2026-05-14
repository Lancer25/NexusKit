#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/screen/export.h>

namespace nexus::screen {

namespace detail {
class ScreenCapturerStorage;
}

struct ScreenBackendInfo {
    std::string name = "screen";
    bool available = false;
    std::string description;
};

enum class ScreenPixelFormat {
    unknown,
    bgra,
    rgba
};

struct ScreenFrame {
    int width = 0;
    int height = 0;
    ScreenPixelFormat pixel_format = ScreenPixelFormat::unknown;
    std::int64_t timestamp_ms = 0;
    std::vector<std::uint8_t> data;
};

struct ScreenCaptureOptions {
    bool include_cursor = false;
};

NEXUS_SCREEN_API ScreenBackendInfo screen_backend_info();

class NEXUS_SCREEN_API ScreenCapturer {
public:
    ScreenCapturer();
    ScreenCapturer(const ScreenCapturer&) = delete;
    ScreenCapturer& operator=(const ScreenCapturer&) = delete;
    ScreenCapturer(ScreenCapturer&& other) noexcept;
    ScreenCapturer& operator=(ScreenCapturer&& other) noexcept;
    ~ScreenCapturer();

    static Result<ScreenCapturer> create(const ScreenCaptureOptions& options = {});

    bool is_available() const;
    Result<ScreenFrame> capture_primary();
    Status close();

private:
    explicit ScreenCapturer(std::unique_ptr<detail::ScreenCapturerStorage> storage);

    std::unique_ptr<detail::ScreenCapturerStorage> storage_;
};

} // namespace nexus::screen
