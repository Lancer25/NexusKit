#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nexus/camera/export.h>

/// Camera capture types: device descriptors, frames, and capture options.
namespace nexus::camera {

/// A camera device descriptor.
struct NEXUS_CAMERA_API CameraDevice {
    /// Opaque backend identifier.  Do not persist across runs.
    std::string id;
    /// Human-readable device name.
    std::string name;
    /// Device path (e.g. /dev/video0 on Linux, device path on Windows).
    std::string path;
    /// USB vendor id.  0 when not available.
    unsigned int vid = 0;
    /// USB product id.  0 when not available.
    unsigned int pid = 0;
};

/// Camera control kinds (UVC/V4L2 standard controls).
enum class NEXUS_CAMERA_API CameraControlKind {
    /// Image brightness.
    kBrightness = 0,
    /// Image contrast.
    kContrast,
    /// Image saturation.
    kSaturation,
    /// Image hue.
    kHue,
    /// Edge enhancement or image sharpness.
    kSharpness,
    /// Gamma correction.
    kGamma,
    /// White balance color temperature.
    kWhiteBalance,
    /// Backlight compensation.
    kBacklightCompensation,
    /// Sensor or analog gain.
    kGain,
    /// Lens focus position.
    kFocus,
    /// Optical or digital zoom.
    kZoom,
    /// Exposure time or exposure value.
    kExposure,
    /// Iris or aperture position.
    kIris,
    /// Pan position.
    kPan,
    /// Tilt position.
    kTilt,
    /// Roll position.
    kRoll,
    /// Automatic focus mode.
    kAutoFocus,
    /// Automatic white balance mode.
    kAutoWhiteBalance,
    /// Automatic exposure mode.
    kAutoExposure,
    /// Anti-flicker power line frequency.
    kPowerLineFrequency,
};

/// Range and capabilities for a camera control.
struct NEXUS_CAMERA_API CameraControlRange {
    /// Minimum allowed value.
    int minimum = 0;
    /// Maximum allowed value.
    int maximum = 0;
    /// Step size between values.
    int step = 1;
    /// Default value.
    int default_value = 0;
    /// Whether the control supports automatic mode.
    bool supports_auto = false;
    /// Whether the control is available on this device.
    bool available = false;
};

/// Pixel formats for captured camera frames.
enum class NEXUS_CAMERA_API PixelFormat {
    /// Unknown or unsupported format.
    kUnknown = 0,
    /// Motion JPEG compressed frames.
    kMJPEG,
    /// YUYV 4:2:2 uncompressed frames.
    kYUYV,
};

/// A captured camera frame.
struct NEXUS_CAMERA_API CameraFrame {
    /// Monotonic capture timestamp in milliseconds.
    std::int64_t timestamp_ms = 0;
    /// Frame pixel data.
    std::vector<std::uint8_t> data;
    /// Frame width in pixels.
    unsigned int width = 0;
    /// Frame height in pixels.
    unsigned int height = 0;
    /// Pixel format of the frame data.
    PixelFormat pixel_format = PixelFormat::kUnknown;
};

/// Camera capture configuration.
struct NEXUS_CAMERA_API CameraCaptureOptions {
    /// Device id to capture from.  Empty string = first available device.
    std::string device_id;
    /// Desired frame width.  0 = device default.
    unsigned int width = 640;
    /// Desired frame height.  0 = device default.
    unsigned int height = 480;
    /// Desired frame rate.  0 = device default.
    unsigned int frame_rate = 30;
};

} // namespace nexus::camera
