# Architecture

NexusKit is organized as focused CMake targets with public headers under `include/nexus` and implementation files under `src`.

## Dependency Layer

Third-party CMake recipes live under `cmake/deps`. Module and test CMake files include dependency recipes instead of declaring `FetchContent` directly. This keeps proxy, Git, and `NEXUS_BUILD_DEPS` behavior consistent across the project.

Recipes are intentionally opt-in. The top-level build only activates heavyweight source builds such as OpenSSL, hidapi, PortAudio, and FFmpeg when their `NEXUS_BUILD_*` option is enabled. Future modules will include the lightweight recipes they actually link against.

## Modules

- Current Phase 1: `nexus_core` is the lowest-level module and depends only on C++17 and minimal platform SDKs.
- Current Phase 3: `nexus_log` provides logging and depends on `nexus_core` plus spdlog as a private backend.
- Current Phase 3: `nexus_common` provides shared utility components such as JSON and XML and depends on `nexus_core` plus private backend libraries.
- Current Phase 4: `nexus_net` provides networking components. It currently includes HTTP, synchronous TCP, synchronous UDP, and synchronous WebSocket utilities, with diagnostics routed through `nexus_log`.
- Current Phase 5: `nexus_usb` provides USB device discovery and HID-backed report I/O facade APIs and will expand toward UVC and hotplug support.
- Current Phase 5: `nexus_hid` provides HID device enumeration through a private hidapi backend.
- Planned: `nexus_media` will provide FFmpeg-based media processing.
- Planned: `nexus_screen` will provide desktop and window capture.

## Rules

Public headers must not expose platform headers unless the API is explicitly platform-specific.
Owning resources must use RAII.
Recoverable failures should return `nexus::Status` or `nexus::Result<T>`.
