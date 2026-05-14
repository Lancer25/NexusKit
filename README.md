# NexusKit

NexusKit is a cross-platform C++17 component library for Windows and Linux. It is intended to provide reusable modules beyond the core for logging, common data helpers, networking, USB/HID, media processing, and screen capture.

The project is a new implementation inspired by earlier internal component-library experience. It does not preserve legacy APIs or private SDK dependencies.

## Status

NexusKit is in early development. The current milestone establishes the CMake project skeleton, `nexus_core`, `nexus_log`, `nexus_common`, `nexus_net`, `nexus_hid`, `nexus_usb`, the first `nexus_media` slice, tests, examples, and documentation.

## Requirements

- CMake 3.24 or newer
- C++17 compiler
- MSVC 2022 on Windows, or GCC/Clang on Linux
- Git for dependency downloads

## Quick Start

```powershell
cmake --preset windows-msvc-debug
cmake --build --preset windows-msvc-debug
ctest --preset windows-msvc-debug
```

If dependency downloads need the local proxy:

```powershell
$env:HTTP_PROXY="http://127.0.0.1:7897"
$env:HTTPS_PROXY="http://127.0.0.1:7897"
cmake --preset windows-msvc-debug
```

## Modules

- Current Phase 1: `nexus_core`: status/result types, versioning, platform foundations.
- Current Phase 3: `nexus_log`: logging API with a private spdlog backend.
- Current Phase 3: `nexus_common`: JSON and XML utilities with private nlohmann_json and pugixml backends.
- Current Phase 4: `nexus_net`: HTTP, TCP, UDP, and WebSocket utilities with private cpp-httplib/Asio/websocketpp backends and NexusKit diagnostics.
- Current Phase 5: `nexus_usb`: USB device discovery and HID-backed report I/O facade.
- Current Phase 5: `nexus_hid`: HID device enumeration with a private hidapi backend.
- Current Phase 6: `nexus_media`: media backend discovery, metadata probing, packet reading, audio/video decoding, first-frame conversion, and lightweight WAV/PPM frame writers.
- Planned: `nexus_screen`: desktop and window capture.
