#pragma once

#include <functional>
#include <memory>

#include <nexus/camera/export.h>
#include <nexus/camera/types.h>
#include <nexus/core/result.h>
#include <nexus/core/status.h>

/// Camera capture: enumerate USB camera devices and capture video frames.
///
/// On Windows the backend uses libuvc (USB Video Class library).
/// On Linux the backend uses V4L2 (Video4Linux2 kernel API).
/// Backend headers are private; public headers only expose NexusKit types.
namespace nexus::camera {

namespace detail {
class CameraCapturerStorage;
}

/// Move-only RAII camera capturer.
///
/// Created via `create(options)`.  Start capturing with `start(handler)`
/// which delivers `Result<CameraFrame>` on an internal capture thread.
class NEXUS_CAMERA_API CameraCapturer {
public:
    /// Callback type for captured video frames.
    using DataHandler = std::function<void(Result<CameraFrame>)>;

    /// Moves a camera capturer handle.
    CameraCapturer(CameraCapturer&& other) noexcept;
    /// Moves a camera capturer handle.
    CameraCapturer& operator=(CameraCapturer&& other) noexcept;
    /// Stops capture and releases the capturer if needed.
    ~CameraCapturer();

    /// Creates a capturer with the given options.
    ///
    /// @param options Capture configuration (device, resolution, frame rate).
    /// @return A camera capturer on success.
    /// @retval kFailedPrecondition when no backend is available.
    /// @retval kNotFound when no camera devices are found.
    /// @retval kInvalidArgument when options are invalid.
    static Result<CameraCapturer> create(CameraCaptureOptions options = {});

    /// Enumerates available camera devices.
    ///
    /// @return A list of camera device descriptors.
    /// @retval kFailedPrecondition when no backend is available.
    static Result<std::vector<CameraDevice>> enumerate_devices();

    /// Returns the default camera device.
    ///
    /// Equivalent to `enumerate_devices()` and returning the first entry.
    /// @retval kNotFound when no camera devices are found.
    static Result<CameraDevice> default_device();

    /// Starts camera capture.  Frames are delivered via `handler` on an
    /// internal capture thread.  Must not already be capturing.
    ///
    /// @param handler Callback receiving each captured frame.
    /// @retval kFailedPrecondition when already capturing.
    /// @retval kUnavailable on backend failure.
    Status start(DataHandler handler);

    /// Stops capturing.  Idempotent; safe to call multiple times.
    Status stop();

    /// True while actively capturing frames.
    bool is_capturing() const;

    /// Lists controls supported by this device.
    ///
    /// @return A list of supported control kinds.
    /// @retval kFailedPrecondition when device id is invalid.
    std::vector<CameraControlKind> supported_controls() const;

    /// Gets the current value of a camera control.
    ///
    /// @param kind The control to query.
    /// @param auto_enabled Output: true if the control is in automatic mode.
    /// @return The current control value.
    /// @retval kNotFound when the control is not supported.
    Result<int> get_control(CameraControlKind kind, bool& auto_enabled) const;

    /// Sets a camera control value.
    ///
    /// @param kind The control to set.
    /// @param value The value to set.
    /// @param auto_enabled True to enable automatic mode.
    /// @retval kNotFound when the control is not supported.
    Status set_control(CameraControlKind kind, int value, bool auto_enabled = false) const;

    /// Gets the supported range of a camera control.
    ///
    /// @param kind The control to query.
    /// @return The control range.
    /// @retval kNotFound when the control is not supported.
    Result<CameraControlRange> get_control_range(CameraControlKind kind) const;

private:
    explicit CameraCapturer(std::unique_ptr<detail::CameraCapturerStorage> storage);

    std::unique_ptr<detail::CameraCapturerStorage> storage_;
};

} // namespace nexus::camera
