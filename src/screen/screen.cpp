#include <nexus/screen/screen.h>

#include "screen_frame_crop.h"
#include "screen_private.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace nexus::screen {

namespace {

Status unavailable_status() {
    return Status(StatusCode::kFailedPrecondition, "screen capture backend is not available");
}

} // namespace

namespace detail {

namespace {

class UnavailableScreenCapturerStorage final : public ScreenCapturerStorage {
public:
    explicit UnavailableScreenCapturerStorage(ScreenCaptureOptions options)
        : options_(options) {}

    bool is_available() const override {
        return false;
    }

    Result<std::vector<ScreenDisplay>> displays() const override {
        return unavailable_status();
    }

    Result<std::vector<ScreenWindow>> windows() const override {
        return unavailable_status();
    }

    Result<ScreenFrame> capture_primary() override {
        return unavailable_status();
    }

    Result<ScreenFrame> capture_display(const std::string&) override {
        return unavailable_status();
    }

    Result<ScreenFrame> capture_window(const std::string&) override {
        return unavailable_status();
    }

    void close() override {}

private:
    ScreenCaptureOptions options_;
};

} // namespace

#if !defined(NEXUS_SCREEN_WITH_DXGI) && !defined(NEXUS_SCREEN_WITH_X11)
std::unique_ptr<ScreenCapturerStorage> create_screen_capturer_storage(
    const ScreenCaptureOptions& options) {
    return std::make_unique<UnavailableScreenCapturerStorage>(options);
}

ScreenBackendInfo query_screen_backend_info() {
    ScreenBackendInfo info;
    info.description = "No screen capture backend is available in this build";
    return info;
}
#endif

} // namespace detail

ScreenBackendInfo screen_backend_info() {
    return detail::query_screen_backend_info();
}

std::string screen_pixel_format_name(ScreenPixelFormat format) {
    switch (format) {
    case ScreenPixelFormat::bgra:
        return "bgra";
    case ScreenPixelFormat::rgba:
        return "rgba";
    case ScreenPixelFormat::unknown:
    default:
        return {};
    }
}

Status write_ppm_file(const std::filesystem::path& path, const ScreenFrame& frame) {
    if (path.empty()) {
        return Status::invalid_argument("PPM path cannot be empty");
    }
    if (frame.width <= 0 || frame.height <= 0 || frame.data.empty()) {
        return Status::invalid_argument("PPM writer requires a non-empty screen frame");
    }
    if (frame.pixel_format != ScreenPixelFormat::bgra) {
        return Status::invalid_argument("PPM writer requires BGRA screen frames");
    }

    const auto pixel_count = static_cast<std::size_t>(frame.width) *
                             static_cast<std::size_t>(frame.height);
    const auto expected_size = pixel_count * 4u;
    if (frame.data.size() < expected_size) {
        return Status::invalid_argument("PPM screen frame data is smaller than expected");
    }

    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return Status::invalid_argument("PPM path cannot be opened");
    }

    output << "P6\n" << frame.width << " " << frame.height << "\n255\n";
    for (std::size_t pixel = 0; pixel < pixel_count; ++pixel) {
        const auto offset = pixel * 4u;
        output.put(static_cast<char>(frame.data[offset + 2u]));
        output.put(static_cast<char>(frame.data[offset + 1u]));
        output.put(static_cast<char>(frame.data[offset]));
    }

    if (!output) {
        return Status::internal("PPM write failed");
    }

    return Status::ok_status();
}

ScreenCapturer::ScreenCapturer() = default;

ScreenCapturer::ScreenCapturer(ScreenCapturer&& other) noexcept = default;

ScreenCapturer& ScreenCapturer::operator=(ScreenCapturer&& other) noexcept = default;

ScreenCapturer::~ScreenCapturer() = default;

Result<ScreenCapturer> ScreenCapturer::create(const ScreenCaptureOptions& options) {
    return ScreenCapturer(detail::create_screen_capturer_storage(options));
}

bool ScreenCapturer::is_available() const {
    return storage_ && storage_->is_available();
}

Result<std::vector<ScreenDisplay>> ScreenCapturer::displays() const {
    if (!is_available()) {
        return unavailable_status();
    }

    return storage_->displays();
}

Result<std::vector<ScreenWindow>> ScreenCapturer::windows() const {
    if (!is_available()) {
        return unavailable_status();
    }

    return storage_->windows();
}

Result<ScreenFrame> ScreenCapturer::capture_primary() {
    if (!is_available()) {
        return unavailable_status();
    }

    return storage_->capture_primary();
}

Result<ScreenFrame> ScreenCapturer::capture_display(const std::string& display_id) {
    if (!is_available()) {
        return unavailable_status();
    }

    return storage_->capture_display(display_id);
}

Result<ScreenFrame> ScreenCapturer::capture_region(const ScreenCaptureRegion& region) {
    if (region.width <= 0 || region.height <= 0) {
        return Status::invalid_argument("screen capture region dimensions must be positive");
    }
    if (region.x < 0 || region.y < 0) {
        return Status::invalid_argument("screen capture region origin must be non-negative");
    }
    if (!is_available()) {
        return unavailable_status();
    }

    auto source = region.display_id.empty() ? capture_primary() : capture_display(region.display_id);
    if (!source.ok()) {
        return source.status();
    }

    return detail::crop_frame_bgra(source.value(), region);
}

Result<ScreenFrame> ScreenCapturer::capture_window(const std::string& window_id) {
    if (window_id.empty()) {
        return Status::invalid_argument("screen window id cannot be empty");
    }
    if (!is_available()) {
        return unavailable_status();
    }

    return storage_->capture_window(window_id);
}

Status ScreenCapturer::close() {
    if (storage_) {
        storage_->close();
        storage_.reset();
    }

    return Status::ok_status();
}

ScreenCapturer::ScreenCapturer(std::unique_ptr<detail::ScreenCapturerStorage> storage)
    : storage_(std::move(storage)) {}

} // namespace nexus::screen
