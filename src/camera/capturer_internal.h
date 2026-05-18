#pragma once

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include <nexus/camera/capturer.h>
#include <nexus/core/result.h>

namespace nexus::camera {
namespace detail {

/// Base class for platform-specific capturer storage.
class CameraCapturerStorage {
public:
    CameraCaptureOptions options;
    std::atomic<bool> capturing{false};

    explicit CameraCapturerStorage(CameraCaptureOptions opts)
        : options(std::move(opts)) {}

    virtual ~CameraCapturerStorage() = default;

    virtual Status start_capture(CameraCapturer::DataHandler handler) = 0;
    virtual Status stop_capture() = 0;
};

// Platform backend factories — defined in per-platform TU.

#if defined(NEXUS_CAMERA_HAS_LIBUVC)
Result<std::vector<CameraDevice>> uvc_enumerate_devices();
Result<std::unique_ptr<CameraCapturerStorage>>
uvc_create_storage(CameraCaptureOptions options);
std::vector<CameraControlKind> uvc_supported_controls(const std::string& device_id);
Result<int> uvc_get_control(const std::string& device_id, CameraControlKind kind, bool& auto_enabled);
Status uvc_set_control(const std::string& device_id, CameraControlKind kind, int value, bool auto_enabled);
Result<CameraControlRange> uvc_get_control_range(const std::string& device_id, CameraControlKind kind);
#endif

#if defined(NEXUS_CAMERA_HAS_V4L2)
Result<std::vector<CameraDevice>> v4l2_enumerate_devices();
Result<std::unique_ptr<CameraCapturerStorage>>
v4l2_create_storage(CameraCaptureOptions options);
std::vector<CameraControlKind> v4l2_supported_controls(const std::string& device_path);
Result<int> v4l2_get_control(const std::string& device_path, CameraControlKind kind, bool& auto_enabled);
Status v4l2_set_control(const std::string& device_path, CameraControlKind kind, int value, bool auto_enabled);
Result<CameraControlRange> v4l2_get_control_range(const std::string& device_path, CameraControlKind kind);
#endif

} // namespace detail
} // namespace nexus::camera
