# Architecture

NexusKit is organized as focused CMake targets with public headers under `include/nexus` and implementation files under `src`.

## Dependency Layer

Third-party CMake recipes live under `cmake/deps`. Module and test CMake files include dependency recipes instead of declaring `FetchContent` directly. This keeps proxy, Git, and `NEXUS_BUILD_DEPS` behavior consistent across the project.

Recipes are intentionally opt-in. The top-level build only activates heavyweight source builds such as OpenSSL, hidapi, PortAudio, and FFmpeg when their `NEXUS_BUILD_*` option is enabled. Future modules will include the lightweight recipes they actually link against.

## Modules

- Current Phase 1: `nexus_core` is the lowest-level module and depends only on C++17 and minimal platform SDKs.
- Planned: `nexus_log` will provide logging and depend on `nexus_core` and spdlog.
- Planned: `nexus_common` will provide JSON, XML, crypto, and utility helpers.
- Planned: `nexus_net` will provide TCP, UDP, HTTP, and WebSocket.
- Planned: `nexus_usb` will provide USB and UVC device support.
- Planned: `nexus_hid` will provide HID access through hidapi.
- Planned: `nexus_media` will provide FFmpeg-based media processing.
- Planned: `nexus_screen` will provide desktop and window capture.

## Rules

Public headers must not expose platform headers unless the API is explicitly platform-specific.
Owning resources must use RAII.
Recoverable failures should return `nexus::Status` or `nexus::Result<T>`.
