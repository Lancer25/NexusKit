# Design Spec: nexus_camera — USB Camera Capture

## Motivation

NexusKit lacks camera capture capability. Users need to enumerate USB camera devices and capture video frames on Windows and Linux, following the same cross-platform, backend-private patterns as `nexus_audio` and `nexus_screen`. The primary use case is USB UVC camera capture for MJPEG and YUYV streams.

## Design Goals

1. Cross-platform USB camera device enumeration (Windows via libuvc, Linux via V4L2)
2. Frame capture from a selected camera delivering frames via callback on an internal thread
3. Configurable resolution (width, height) and frame rate
4. Backend libraries (libuvc, V4L2 ioctl) kept entirely private
5. Consistent with existing NexusKit patterns: `nexus::Result<T>`, `nexus::Status`, RAII, move-only handles

## Out of Scope (for initial phase)

- Camera controls (brightness, contrast, zoom, focus, etc.)
- Pixel format selection — auto-negotiate MJPEG first, fall back to YUYV
- Multiple simultaneous camera instances
- Camera hotplug event callbacks
- Audio+video synchronized capture

## Public API

### `include/nexus/camera/types.h`

```cpp
namespace nexus::camera {

/// Camera device descriptor.
struct CameraDevice {
    /// Opaque backend id. Do not persist across runs.
    std::string id;
    /// Human-readable device name.
    std::string name;
    /// Device path (e.g. /dev/video0 on Linux, device path on Windows).
    std::string path;
    /// USB vendor id (0 if unknown).
    unsigned int vid = 0;
    /// USB product id (0 if unknown).
    unsigned int pid = 0;
};

/// Pixel formats for captured frames.
enum class PixelFormat {
    /// Unknown or unsupported format.
    kUnknown = 0,
    /// Motion JPEG compressed frames.
    kMJPEG,
    /// YUYV 4:2:2 uncompressed frames.
    kYUYV,
};

/// Captured camera frame.
struct CameraFrame {
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

/// Capture configuration.
struct CameraCaptureOptions {
    /// Device id to capture from. Empty = first available device.
    std::string device_id;
    /// Desired frame width (0 = device default, typically 640).
    unsigned int width = 640;
    /// Desired frame height (0 = device default, typically 480).
    unsigned int height = 480;
    /// Desired frame rate (0 = device default, typically 30).
    unsigned int frame_rate = 30;
};

} // namespace nexus::camera
```

### `include/nexus/camera/capturer.h`

```cpp
namespace nexus::camera {

class NEXUS_CAMERA_API CameraCapturer {
public:
    using DataHandler = std::function<void(Result<CameraFrame>)>;

    CameraCapturer(CameraCapturer&&) noexcept;
    CameraCapturer& operator=(CameraCapturer&&) noexcept;
    ~CameraCapturer();

    /// Creates a capturer with the given options.
    static Result<CameraCapturer> create(CameraCaptureOptions options = {});

    /// Enumerates available camera devices.
    static Result<std::vector<CameraDevice>> enumerate_devices();

    /// Starts capturing. Frames are delivered via handler on an internal thread.
    Status start(DataHandler handler);

    /// Stops capturing. Idempotent.
    Status stop();

    /// True while actively capturing frames.
    bool is_capturing() const;

private:
    explicit CameraCapturer(std::unique_ptr<detail::CameraCapturerStorage> storage);
    std::unique_ptr<detail::CameraCapturerStorage> storage_;
};

} // namespace nexus::camera
```

## Backend Strategy

### Windows: libuvc

- Use libuvc (cross-platform USB Video Class library)
- `uvc_find_devices()` → `uvc_get_device_descriptor()` for enumeration
- Retrieve vid/pid from `uvc_device_descriptor_t`
- Open device with `uvc_open()`, negotiate format with `uvc_get_stream_ctrl_format_size()`
- Start streaming via `uvc_start_streaming()` with frame callback
- Format negotiation: try MJPEG first, then YUYV, at requested resolution/frame rate
- libuvc added as FetchContent dependency with `NEXUS_BUILD_LIBUVC` option

### Linux: V4L2

- Use Linux kernel V4L2 API (`videodev2.h`, `ioctl`)
- Enumerate devices by scanning `/dev/video*`, querying `VIDIOC_QUERYCAP` for `V4L2_CAP_VIDEO_CAPTURE`
- Retrieve device name from `v4l2_capability.card`
- Configure format via `VIDIOC_S_FMT` (try MJPEG first, then YUYV)
- Set frame rate via `VIDIOC_S_PARM` with `V4L2_CAP_TIMEPERFRAME`
- Allocate mmap buffers via `VIDIOC_REQBUFS` → `VIDIOC_QUERYBUF` → `mmap` → `VIDIOC_QBUF`
- Start streaming via `VIDIOC_STREAMON`
- Capture loop: `select()` with timeout → `VIDIOC_DQBUF` → build `CameraFrame` → callback → `VIDIOC_QBUF`
- Stop: `VIDIOC_STREAMOFF` → `munmap` → `close(fd)`

### Build Integration

- New CMake option: `NEXUS_ENABLE_CAMERA` (default OFF)
- Windows: FetchContent libuvc (requires libusb), link `libuvc`
- Linux: No extra dependencies (V4L2 is kernel API via system headers)
- `NEXUS_BUILD_LIBUVC` option to control libuvc source build

## Module Layout

```
include/nexus/camera/
    export.h
    types.h          — CameraDevice, CameraFrame, CameraCaptureOptions, PixelFormat
    capturer.h       — CameraCapturer class

src/camera/
    CMakeLists.txt
    capturer.cpp         — public methods, platform dispatch
    uvc_capturer.cpp     — Windows libuvc backend (detail::CameraCapturerStorage)
    v4l2_capturer.cpp    — Linux V4L2 backend (detail::CameraCapturerStorage)

tests/camera/
    camera_tests.cpp
```

## Error Model

- No supported backend → `StatusCode::kFailedPrecondition`
- Device not found → `StatusCode::kNotFound`
- Invalid options → `StatusCode::kInvalidArgument`
- Capture already running → `StatusCode::kFailedPrecondition`
- Platform API failure → `StatusCode::kUnavailable`
- No camera devices available → `StatusCode::kNotFound`
