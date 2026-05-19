# Architecture

NexusKit is organized as focused CMake targets with public headers under `include/nexus` and implementation files under `src`.

## Dependency Layer

Third-party CMake recipes live under `cmake/deps`. Module and test CMake files include dependency recipes instead of declaring `FetchContent` directly. This keeps proxy, Git, and `NEXUS_BUILD_DEPS` behavior consistent across the project.

Recipes are intentionally opt-in. The top-level build only activates heavyweight source builds such as OpenSSL, hidapi, PortAudio, and FFmpeg when their `NEXUS_BUILD_*` option is enabled. Future modules will include the lightweight recipes they actually link against.

## Modules

- `nexus_core`: The lowest-level module — `StatusCode`, `Status`, and `Result<T>` types. Depends only on C++17 and minimal platform SDKs.
- `nexus_log`: Logging facade with spdlog as a private backend. Depends on `nexus_core`.
- `nexus_common`: Shared utilities — JSON, XML, string manipulation, time helpers, binary I/O, platform abstractions, math helpers, diagnostic logging, and threading primitives (`Thread`, `Event`). Depends on `nexus_core`, `nexus_log`, and private backend libraries (nlohmann_json, pugixml).
- `nexus_net`: Networking — HTTP client/server, TCP, UDP, WebSocket (client and server), with sync/async methods and optional TLS via OpenSSL. Depends on `nexus_log`, with private backends (cpp-httplib, Asio, websocketpp).
- `nexus_usb`: USB device discovery, HID-backed report I/O, and USB-level hotplug monitoring. Depends on `nexus_hid` and `nexus_log`.
- `nexus_hid`: HID device enumeration through a private hidapi backend. Depends on `nexus_log`.
- `nexus_media`: FFmpeg-backed media processing — backend discovery, metadata probing, packet reading, audio/video decoding, resampling/format conversion, encoding, container muxing, and lightweight WAV/PPM writers. Depends on `nexus_log`.
- `nexus_screen`: Desktop and window capture — Windows DXGI Desktop Duplication and Linux X11, with display enumeration, region/window capture, cursor overlay, and pixel-format helpers. Depends on `nexus_log`.
- `nexus_audio`: Audio input device enumeration and PCM int16 capture via WASAPI (Windows) or PulseAudio (Linux), with volume/mute control. Depends on `nexus_log` and `nexus_common`.
- `nexus_camera`: USB camera device enumeration and video frame capture via libuvc (Windows) or V4L2 (Linux), with camera control support. Depends on `nexus_log` and `nexus_common`.

## Rules

Public headers must not expose platform headers unless the API is explicitly platform-specific.
Owning resources must use RAII.
Recoverable failures should return `nexus::Status` or `nexus::Result<T>`.

## Development Guidance

Public API comments, error semantics, and header boundary rules are defined in `docs/api-style.md`.
Future phase workflow, verification expectations, and the near-term roadmap are defined in `docs/iteration.md`.
