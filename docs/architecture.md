# Architecture

NexusKit is organized as focused CMake targets with public headers under `include/nexus` and implementation files under `src`.

## Dependency Layer

Third-party CMake recipes live under `cmake/deps`. Module and test CMake files include dependency recipes instead of declaring `FetchContent` directly. This keeps proxy, Git, and `NEXUS_BUILD_DEPS` behavior consistent across the project.

Recipes are intentionally opt-in. The top-level build only activates heavyweight source builds such as OpenSSL, hidapi, PortAudio, and FFmpeg when their `NEXUS_BUILD_*` option is enabled. Future modules will include the lightweight recipes they actually link against.

## Modules

- Current Phase 1: `nexus_core` is the lowest-level module and depends only on C++17 and minimal platform SDKs.
- Current Phase 3: `nexus_log` provides logging and depends on `nexus_core` plus spdlog as a private backend.
- Current Phase 3: `nexus_common` provides shared utility components including JSON, XML, string manipulation, time helpers, binary I/O, platform abstractions, math helpers, diagnostic logging, and status factories. It depends on `nexus_core`, `nexus_log`, and private backend libraries.
- Current Phase 4: `nexus_net` provides networking components. It currently includes HTTP, synchronous TCP, synchronous UDP, and synchronous WebSocket utilities, with diagnostics routed through `nexus_log`.
- Current Phase 5: `nexus_usb` provides USB device discovery, HID-backed report I/O facade APIs, and USB-level hotplug monitoring. Future USB work can expand toward UVC and USB audio discovery; HID-level hotplug is not planned.
- Current Phase 5: `nexus_hid` provides HID device enumeration through a private hidapi backend.
- Current Phase 6: `nexus_media` provides FFmpeg-backed media processing including backend discovery, metadata probing, packet reading, audio/video decoding, audio resampling and sample-format conversion, video pixel-format conversion (RGB24, RGBA, BGR24, BGRA), lightweight WAV and PPM frame writers, and stream-type and format-name helpers.
- Current Phase 7: `nexus_screen` provides desktop and window capture through Windows DXGI Desktop Duplication and Linux X11. It supports display enumeration, primary/display-id/region capture, visible-window enumeration and capture with metadata (containing display, process id, active flag) on both Windows and Linux X11, optional cursor overlay on Windows, a PPM writer for BGRA frames, and pixel-format name helpers.
- `nexus_audio`: audio input device enumeration and PCM int16 capture via WASAPI (Windows) or PulseAudio (Linux). The backend is private; the public API exposes `AudioCapturer` with callback-driven frame delivery on an internal capture thread.
- `nexus_camera`: USB camera device enumeration and video frame capture via libuvc (Windows) or V4L2 (Linux). The backend is private; the public API exposes `CameraCapturer` with callback-driven frame delivery on an internal capture thread.

## Rules

Public headers must not expose platform headers unless the API is explicitly platform-specific.
Owning resources must use RAII.
Recoverable failures should return `nexus::Status` or `nexus::Result<T>`.

## Development Guidance

Public API comments, error semantics, and header boundary rules are defined in `docs/api-style.md`.
Future phase workflow, verification expectations, and the near-term roadmap are defined in `docs/iteration.md`.
