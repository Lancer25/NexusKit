# Implementation Plan: nexus_camera — USB Camera Capture (Phase 12B)

## Overview

Add a new `nexus_camera` optional module providing cross-platform USB camera device enumeration and video frame capture via libuvc (Windows) and V4L2 (Linux).

## Stages

### Stage 1: Module skeleton, types, and CMake

- Create `include/nexus/camera/export.h`, `types.h`, `capturer.h`
- Create `src/camera/CMakeLists.txt` with platform backend selection
- Add `NEXUS_ENABLE_CAMERA` option to root `CMakeLists.txt` (default OFF)
- Add `nexus_camera` library target and `nexus::camera` alias
- Link `nexus::core`, `nexus::common`
- Windows: FetchContent libuvc, link `libuvc`
- Linux: no extra link dependencies (V4L2 is kernel API)
- Stub out `CameraCapturer` with all methods returning `kUnavailable` when no backend

### Stage 2: Windows libuvc backend

- Implement `detail::CameraCapturerStorage` in `src/camera/uvc_capturer.cpp`
- Device enumeration via `uvc_find_devices()` + `uvc_get_device_descriptor()`
- Format negotiation: `uvc_get_stream_ctrl_format_size()` with MJPEG → YUYV fallback
- Frame callback via `uvc_start_streaming()`
- Internal capture thread using `nexus::common::Thread`
- Cleanup: `uvc_stop_streaming()` → `uvc_close()` → `uvc_unref_device()` → `uvc_exit()`

### Stage 3: Linux V4L2 backend

- Implement `detail::CameraCapturerStorage` in `src/camera/v4l2_capturer.cpp`
- Device enumeration via scanning `/dev/video*`, `VIDIOC_QUERYCAP`
- Format config via `VIDIOC_S_FMT` (MJPEG first, YUYV fallback)
- Frame rate via `VIDIOC_S_PARM`
- mmap buffer lifecycle (`VIDIOC_REQBUFS` → `mmap` → `VIDIOC_QBUF`)
- Capture loop: `select()` → `VIDIOC_DQBUF` → callback → `VIDIOC_QBUF`
- Internal capture thread using `nexus::common::Thread`

### Stage 4: Tests

- `tests/camera/camera_tests.cpp` with Catch2
- Test: enumerate devices returns list (may be empty without physical camera)
- Test: create with default options succeeds (when camera available)
- Test: start/stop capture lifecycle
- Test: captured frames have valid data and dimensions
- Test: stop is idempotent
- Test: reject double start

### Stage 5: Documentation

- Add `docs/modules/camera.md` module reference
- Update `docs/architecture.md` with nexus_camera description
- Update `docs/iteration.md` completed work
- Update `CHANGELOG.md`
- Update `README.md` module table
