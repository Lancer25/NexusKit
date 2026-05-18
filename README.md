# NexusKit

A cross-platform C++17 component library for device communication, networking, media processing, and screen capture on Windows and Linux.

NexusKit provides reusable, RAII-style modules with public headers free of platform SDK and third-party dependency leakage. All I/O types offer both synchronous (blocking) and asynchronous (callback-driven) operation modes.

## Status

NexusKit is approaching its 0.1.0 milestone. All core modules are implemented with tests, documentation, and examples. The library is under active development on `main`.

## Features

- **Cross-platform** — Windows (MSVC 2022) and Linux (GCC/Clang), with platform abstractions isolated in implementation files
- **Sync + async** — every network I/O type provides both blocking and callback-driven async operations
- **TLS support** — HTTPS and WSS via optional OpenSSL integration (`NEXUS_NET_HAS_TLS`)
- **RAII everywhere** — resource-owning types manage lifetimes automatically
- **Clean headers** — public API headers never expose backend libraries (Asio, cpp-httplib, websocketpp, FFmpeg, hidapi, spdlog)
- **Consistent error handling** — `nexus::Status` / `nexus::Result<T>` for all recoverable failures

## Requirements

- CMake 3.24+
- C++17 compiler (MSVC 2022, GCC 9+, Clang 10+)
- Git (for FetchContent dependency downloads)
- Optional: OpenSSL (for TLS support), FFmpeg (for media module), MSYS2 UCRT64 + NASM (for FFmpeg from source on Windows)

## Quick Start

```powershell
# Configure
cmake --preset windows-msvc-debug

# Build
cmake --build --preset windows-msvc-debug

# Run all tests
ctest --preset windows-msvc-debug
```


Optional modules are enabled via CMake variables:

```powershell
cmake --preset windows-msvc-debug -DNEXUS_ENABLE_MEDIA=ON -DNEXUS_ENABLE_USB=ON
```

## Development Guides

- [Architecture](docs/architecture.md): module layout, dependency boundaries, and high-level rules.
- [Build Guide](docs/build.md): presets, dependency fetching, proxy settings, and install layout.
- [API Style Guide](docs/api-style.md): public header comments, error semantics, and backend boundary rules.
- [Iteration Guide](docs/iteration.md): phase workflow, verification expectations, and near-term roadmap.

## Modules

| Module | Description | Default |
|--------|-------------|---------|
| `nexus_core` | Status/result types, versioning, and platform detection | ON |
| `nexus_log` | Diagnostic logging API (spdlog backend, headers private) | ON |
| `nexus_common` | Strings, time, binary I/O, threading (`Thread`, `Event`), JSON, XML, platform helpers | ON |
| `nexus_net` | HTTP client (sync/async, TLS), TCP client, UDP socket, WebSocket client/server (sync/async, TLS) | ON |
| `nexus_screen` | Display enumeration, screen/window/region capture (Windows DXGI, Linux X11/Xrandr) | ON |
| `nexus_usb` | USB device enumeration, HID report I/O facade, USB hotplug monitoring | OFF |
| `nexus_hid` | HID device enumeration and feature-report documentation | OFF |
| `nexus_media` | Media format probing, audio/video decoding, encoding (AAC/H264), muxing (MP4/MPEG-PS) | OFF |
| `nexus_audio` | Audio input device enumeration and PCM int16 capture (WASAPI/PulseAudio) | OFF |
| `nexus_camera` | USB camera device enumeration and video frame capture (libuvc/V4L2) | OFF |

## Examples

| Example | Module | Description |
|---------|--------|-------------|
| `nexus_example_core_status` | nexus_core | Demonstrates Status and Result error-handling patterns |
| `nexus_example_screen_capture` | nexus_screen | Captures a display/window/region frame to a PPM image |
| `nexus_example_usb_hotplug` | nexus_usb | Lists USB devices and watches arrival/removal events |
| `nexus_example_ffmpeg_probe` | nexus_media | Probes a media file for stream metadata |

Screen capture usage:

```powershell
nexus_example_screen_capture --list-displays
nexus_example_screen_capture --list-windows
nexus_example_screen_capture screen_capture.ppm --cursor
nexus_example_screen_capture window.ppm --window win32-hwnd-123456
nexus_example_screen_capture region.ppm --display dxgi-output-0 --region 100,100,640,360
```

USB hotplug usage:

```powershell
nexus_example_usb_hotplug --list
nexus_example_usb_hotplug --watch --seconds 30
```

## Documentation

- [Architecture](docs/architecture.md) — module layout, dependency boundaries, and design rules
- [Build Guide](docs/build.md) — presets, dependency fetching, proxy settings, and install layout
- [API Style Guide](docs/api-style.md) — public header conventions, error semantics, and backend-boundary rules
- [Iteration Guide](docs/iteration.md) — development workflow, recent work, and roadmap
- [Maintenance Guide](docs/maintenance.md) — daily operations and repository hygiene
- [Module Docs](docs/modules/) — per-module API reference and error model
- [Changelog](CHANGELOG.md) — version history

## License

This project is proprietary internal software. See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.
