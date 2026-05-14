#include <nexus/screen/screen.h>

#include <memory>
#include <utility>

namespace nexus::screen {

namespace {

Status unavailable_status() {
    return Status(StatusCode::kFailedPrecondition, "screen capture backend is not available");
}

} // namespace

namespace detail {

class ScreenCapturerStorage {
public:
    explicit ScreenCapturerStorage(ScreenCaptureOptions options)
        : options_(options) {}

    bool is_available() const {
        return false;
    }

    void close() {}

private:
    ScreenCaptureOptions options_;
};

} // namespace detail

ScreenBackendInfo screen_backend_info() {
    ScreenBackendInfo info;
    info.description = "No screen capture backend is available in this build";
    return info;
}

ScreenCapturer::ScreenCapturer() = default;

ScreenCapturer::ScreenCapturer(ScreenCapturer&& other) noexcept = default;

ScreenCapturer& ScreenCapturer::operator=(ScreenCapturer&& other) noexcept = default;

ScreenCapturer::~ScreenCapturer() = default;

Result<ScreenCapturer> ScreenCapturer::create(const ScreenCaptureOptions& options) {
    return ScreenCapturer(std::make_unique<detail::ScreenCapturerStorage>(options));
}

bool ScreenCapturer::is_available() const {
    return storage_ && storage_->is_available();
}

Result<ScreenFrame> ScreenCapturer::capture_primary() {
    if (!is_available()) {
        return unavailable_status();
    }

    return unavailable_status();
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
