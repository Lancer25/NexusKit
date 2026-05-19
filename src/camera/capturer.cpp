#include "capturer_internal.h"

#include <utility>

#include <nexus/common/logging.h>
#include <nexus/log/logger.h>

namespace nexus::camera {

// --- CameraCapturer ---

CameraCapturer::CameraCapturer(CameraCapturer&& other) noexcept = default;
CameraCapturer& CameraCapturer::operator=(CameraCapturer&& other) noexcept = default;

CameraCapturer::~CameraCapturer() {
    stop();
}

Result<CameraCapturer> CameraCapturer::create(CameraCaptureOptions options) {
    nexus::common::diagnostic_log(log::Level::debug, "CameraCapturer::create");

#if defined(NEXUS_CAMERA_HAS_LIBUVC)
    auto storage = detail::uvc_create_storage(std::move(options));
#elif defined(NEXUS_CAMERA_HAS_V4L2)
    auto storage = detail::v4l2_create_storage(std::move(options));
#else
    auto storage = Result<std::unique_ptr<detail::CameraCapturerStorage>>(
        Status(StatusCode::kFailedPrecondition, "No camera backend available"));
#endif
    if (!storage.ok()) {
        return storage.status();
    }
    return CameraCapturer(std::move(storage).value());
}

Result<std::vector<CameraDevice>> CameraCapturer::enumerate_devices() {
#if defined(NEXUS_CAMERA_HAS_LIBUVC)
    return detail::uvc_enumerate_devices();
#elif defined(NEXUS_CAMERA_HAS_V4L2)
    return detail::v4l2_enumerate_devices();
#else
    return Status(StatusCode::kFailedPrecondition, "No camera backend available");
#endif
}

Result<CameraDevice> CameraCapturer::default_device() {
    auto devices = enumerate_devices();
    if (!devices.ok()) {
        return devices.status();
    }
    const auto& list = devices.value();
    if (list.empty()) {
        return Status(StatusCode::kNotFound, "No camera devices found");
    }
    return list[0];
}

Status CameraCapturer::start(DataHandler handler) {
    if (!storage_) {
        return Status(StatusCode::kFailedPrecondition, "Capturer is closed");
    }
    return storage_->start_capture(std::move(handler));
}

Status CameraCapturer::stop() {
    if (storage_ && storage_->capturing.load()) {
        return storage_->stop_capture();
    }
    return Status::ok_status();
}

bool CameraCapturer::is_capturing() const {
    return storage_ && storage_->capturing.load();
}

// Get effective device id — resolves empty id to first available device path.
std::string effective_device_path(const detail::CameraCapturerStorage& storage) {
    if (!storage.options.device_id.empty()) {
        return storage.options.device_id;
    }
#if defined(NEXUS_CAMERA_HAS_V4L2)
    auto result = detail::v4l2_enumerate_devices();
    if (result.ok() && !result.value().empty()) {
        return result.value()[0].path;
    }
#elif defined(NEXUS_CAMERA_HAS_LIBUVC)
    auto result = detail::uvc_enumerate_devices();
    if (result.ok() && !result.value().empty()) {
        return result.value()[0].id;
    }
#endif
    return {};
}

std::vector<CameraControlKind> CameraCapturer::supported_controls() const {
    if (!storage_) return {};
    auto dev = effective_device_path(*storage_);
    if (dev.empty()) return {};
#if defined(NEXUS_CAMERA_HAS_V4L2)
    return detail::v4l2_supported_controls(dev);
#elif defined(NEXUS_CAMERA_HAS_LIBUVC)
    return detail::uvc_supported_controls(dev);
#else
    return {};
#endif
}

Result<int> CameraCapturer::get_control(CameraControlKind kind, bool& auto_enabled) const {
    if (!storage_)
        return Status(StatusCode::kFailedPrecondition, "Capturer is closed");
    auto dev = effective_device_path(*storage_);
    if (dev.empty())
        return Status(StatusCode::kNotFound, "No camera device available");
#if defined(NEXUS_CAMERA_HAS_V4L2)
    return detail::v4l2_get_control(dev, kind, auto_enabled);
#elif defined(NEXUS_CAMERA_HAS_LIBUVC)
    return detail::uvc_get_control(dev, kind, auto_enabled);
#else
    return Status(StatusCode::kFailedPrecondition, "No camera backend available");
#endif
}

Status CameraCapturer::set_control(CameraControlKind kind, int value, bool auto_enabled) const {
    if (!storage_)
        return Status(StatusCode::kFailedPrecondition, "Capturer is closed");
    auto dev = effective_device_path(*storage_);
    if (dev.empty())
        return Status(StatusCode::kNotFound, "No camera device available");
#if defined(NEXUS_CAMERA_HAS_V4L2)
    return detail::v4l2_set_control(dev, kind, value, auto_enabled);
#elif defined(NEXUS_CAMERA_HAS_LIBUVC)
    return detail::uvc_set_control(dev, kind, value, auto_enabled);
#else
    return Status(StatusCode::kFailedPrecondition, "No camera backend available");
#endif
}

Result<CameraControlRange> CameraCapturer::get_control_range(CameraControlKind kind) const {
    if (!storage_)
        return Status(StatusCode::kFailedPrecondition, "Capturer is closed");
    auto dev = effective_device_path(*storage_);
    if (dev.empty())
        return Status(StatusCode::kNotFound, "No camera device available");
#if defined(NEXUS_CAMERA_HAS_V4L2)
    return detail::v4l2_get_control_range(dev, kind);
#elif defined(NEXUS_CAMERA_HAS_LIBUVC)
    return detail::uvc_get_control_range(dev, kind);
#else
    return Status(StatusCode::kFailedPrecondition, "No camera backend available");
#endif
}

CameraCapturer::CameraCapturer(std::unique_ptr<detail::CameraCapturerStorage> storage)
    : storage_(std::move(storage)) {}

} // namespace nexus::camera
