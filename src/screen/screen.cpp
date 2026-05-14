#include <nexus/screen/screen.h>

#include "screen_private.h"

#include <memory>
#include <utility>

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

    Result<ScreenFrame> capture_primary() override {
        return unavailable_status();
    }

    void close() override {}

private:
    ScreenCaptureOptions options_;
};

} // namespace

#if !defined(NEXUS_SCREEN_WITH_DXGI)
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

Result<ScreenFrame> ScreenCapturer::capture_primary() {
    if (!is_available()) {
        return unavailable_status();
    }

    return storage_->capture_primary();
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
