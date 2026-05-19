# nexus_camera

`nexus_camera` provides cross-platform USB camera device enumeration and video frame capture. On Windows the backend uses libuvc (USB Video Class library); on Linux the backend uses V4L2 (Video4Linux2 kernel API). Platform SDK and third-party headers are kept private — public headers only expose NexusKit types.

## Current API

- `nexus::camera::CameraDevice`: camera device descriptor (id, name, path, vid, pid).
- `nexus::camera::PixelFormat`: frame pixel format enum (kMJPEG, kYUYV).
- `nexus::camera::CameraFrame`: captured video frame (timestamp, data, width, height, pixel_format).
- `nexus::camera::CameraCaptureOptions`: capture configuration (device id, width, height, frame rate).
- `nexus::camera::CameraCapturer`: move-only RAII camera capturer with static enumeration helpers.
- `CameraCapturer::create`: validates options and returns `nexus::Result<CameraCapturer>`.
- `CameraCapturer::enumerate_devices`: returns a list of available camera devices.
- `CameraCapturer::default_device`: returns the first available camera device (equivalent to `enumerate_devices()` and returning the first entry).
- `CameraCapturer::start`: begins capturing, delivering `Result<CameraFrame>` via callback on an internal capture thread.
- `CameraCapturer::stop`: stops capturing (idempotent).
- `CameraCapturer::is_capturing`: true while actively capturing frames.
- `CameraCapturer::supported_controls`: returns a list of supported `CameraControlKind` values.
- `CameraCapturer::get_control`: gets the current value and auto-mode status of a camera control.
- `CameraCapturer::set_control`: sets a camera control value with optional auto-mode.
- `CameraCapturer::get_control_range`: gets the supported range for a camera control.

## Scope

The camera capture module supports enumerating USB camera devices, selecting a device by id (or using the first available device), and capturing video frames delivered via callback. Format negotiation tries MJPEG first, then falls back to YUYV. Capture runs on an internal thread; callers receive frames on that thread. Standard UVC/V4L2 camera controls (brightness, contrast, saturation, focus, zoom, exposure, white balance, etc.) are supported on Linux via V4L2 `VIDIOC_G_CTRL`/`VIDIOC_S_CTRL`/`VIDIOC_QUERYCTRL`. Camera hotplug events are a planned follow-up.

## Backend

On Windows the backend is libuvc which provides USB Video Class device access. On Linux the backend is V4L2 using `ioctl`-based device enumeration, format negotiation, and mmap buffer streaming from `/dev/video*` nodes. These headers are included only from implementation files — users of `nexus_camera` depend on NexusKit headers.

## Diagnostics

Device enumeration, capturer creation, and capture start/stop events write diagnostic messages through `nexus::log::write`. Install a default logger with `nexus::log::set_default_logger` to capture these events. Frame delivery does not log per-frame diagnostics.

## Error Model

- No supported backend → `StatusCode::kFailedPrecondition`
- No camera devices found → `StatusCode::kNotFound`
- Invalid options → `StatusCode::kInvalidArgument`
- Capture already running → `StatusCode::kFailedPrecondition`
- Backend API failure → `StatusCode::kUnavailable`
