#pragma once

#include <memory>

#include <nexus/screen/screen.h>

namespace nexus::screen::detail {

class ScreenCapturerStorage {
public:
    virtual ~ScreenCapturerStorage() = default;

    virtual bool is_available() const = 0;
    virtual Result<ScreenFrame> capture_primary() = 0;
    virtual void close() = 0;
};

std::unique_ptr<ScreenCapturerStorage> create_screen_capturer_storage(
    const ScreenCaptureOptions& options);
ScreenBackendInfo query_screen_backend_info();

} // namespace nexus::screen::detail
