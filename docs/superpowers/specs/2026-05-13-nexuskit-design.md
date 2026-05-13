# NexusKit Design

Date: 2026-05-13

## 1. Project Goals

NexusKit is a new cross-platform C++17 component library for Windows and Linux. It uses the existing HikCommonDll codebase as a source of product experience, but it is not a compatibility rewrite of the old project. Public APIs, module names, directory layout, dependency management, documentation, and tests should follow modern open-source C++ conventions.

The project must provide reusable components for:

- Platform utilities, files, time, strings, threading, errors, and system helpers.
- Asynchronous logging.
- JSON, XML, crypto, and common data utilities.
- TCP, UDP, HTTP, and WebSocket networking.
- USB, UVC, hotplug, and audio device access.
- HID device access.
- Audio/video processing based on locally built FFmpeg.
- Desktop and window capture.

The library must be maintainable as a standalone project. It should support CMake as the only build entry point, MSVC 2022 on Windows, and GCC or Clang on Linux.

## 2. Non-Goals

NexusKit will not preserve old public APIs from CommonUtils, HidTools, UsbTools, MediaTools, or WebSocket. Legacy function names, private SDK concepts, output paths, project files, and module boundaries can be discarded.

The project will not depend on private libraries such as hpr, hlog, HCNetUtils, HCUSBSDK, media_tube, or SCE. Their useful ideas will be replaced by new NexusKit modules or by open-source dependencies.

The project will not keep Visual Studio solution files or QMake project files as first-class build systems. They may remain temporarily during migration, but CMake is authoritative.

## 3. Repository Layout

The target layout is:

```text
NexusKit/
  CMakeLists.txt
  CMakePresets.json
  LICENSE
  README.md
  CHANGELOG.md
  CONTRIBUTING.md
  cmake/
    modules/
    deps/
  docs/
    architecture.md
    build.md
    modules/
  external/
    recipes/
  include/
    nexus/
      core/
      log/
      common/
      net/
      usb/
      hid/
      media/
      screen/
  src/
    core/
    log/
    common/
    net/
    usb/
    hid/
    media/
    screen/
  tests/
  examples/
  tools/
  build/
```

Only stable public headers belong under `include/nexus`. Implementation details belong under `src`. Third-party build recipes belong under `external/recipes` or `cmake/deps`; module code must not contain ad hoc dependency download logic.

## 4. Build System

The build system will be CMake-based and target C++17. It must support:

- MSVC 2022 with the v143 toolset.
- GCC and Clang on Linux.
- Static and shared NexusKit builds.
- Optional module builds.
- Local third-party source builds.
- Installable CMake package exports, so users can consume the library with `find_package(NexusKit CONFIG REQUIRED)`.
- Unified output under `build/`, with install artifacts under `build/install` and distributable artifacts under `build/dist`.

Initial CMake options:

```cmake
NEXUS_BUILD_SHARED
NEXUS_BUILD_TESTS
NEXUS_BUILD_EXAMPLES
NEXUS_BUILD_DOCS

NEXUS_BUILD_DEPS
NEXUS_BUILD_FFMPEG
NEXUS_BUILD_OPENSSL
NEXUS_BUILD_HIDAPI
NEXUS_BUILD_PORTAUDIO

NEXUS_ENABLE_COMMON
NEXUS_ENABLE_NET
NEXUS_ENABLE_USB
NEXUS_ENABLE_HID
NEXUS_ENABLE_MEDIA
NEXUS_ENABLE_SCREEN
```

Each module must expose a CMake target named `nexus::<module>` and an underlying target named `nexus_<module>`, for example `nexus::core` and `nexus_core`.

## 5. Dependency Strategy

NexusKit will build or manage open-source dependencies in a reproducible way. The local proxy for Git and source downloads is `127.0.0.1:7897` and should be documented in `docs/build.md`.

Primary dependencies:

| Dependency | Purpose | Strategy |
| --- | --- | --- |
| spdlog | Logging backend | Fetch/build from source |
| OpenSSL | Crypto and TLS | Build from source when `NEXUS_BUILD_OPENSSL=ON` |
| hidapi | HID access | Build from source |
| asio | TCP/UDP async I/O | Source integration |
| cpp-httplib | HTTP client/server | Source integration |
| websocketpp | WebSocket | Source integration on standalone asio |
| FFmpeg | Audio/video processing | Build locally from source |
| PortAudio | Cross-platform audio I/O | Build locally from source |
| nlohmann_json | JSON utilities | Source integration |
| pugixml | XML utilities | Build from source |
| Catch2 | Tests | Source integration |
| libudev | Linux USB hotplug | Prefer system package detection |
| PulseAudio/libpulse | Linux audio | Prefer system package detection |
| X11/Xrandr/Xinerama | Linux screen capture | Prefer system package detection |

Dependency integration is centralized through CMake recipes under `cmake/deps`. Each recipe must honor `NEXUS_BUILD_DEPS`, document its source/version in `external/recipes`, and use shared Git settings from `NexusGit.cmake` when cloning from Git.

FFmpeg is a first-class dependency. The project should provide a dedicated build recipe for FFmpeg and document required tooling such as NASM/YASM, Perl, MSYS2, or other platform-specific prerequisites. If the final Windows path requires an MSYS2-based FFmpeg build, CMake must detect missing prerequisites and fail with a clear message rather than failing deep inside the build.

## 6. Module Architecture

### nexus_core

`nexus_core` provides shared foundations: result types, status codes, errors, platform detection, filesystem helpers, time utilities, thread primitives, string utilities, dynamic library loading, and small RAII wrappers around operating system resources.

It should depend only on the C++17 standard library and minimal platform SDKs.

### nexus_log

`nexus_log` provides synchronous and asynchronous logging built on spdlog. The public API should not expose spdlog types unless there is a strong reason. It should support console sinks, rotating file sinks, log levels, named loggers, and structured context fields where practical.

### nexus_common

`nexus_common` contains higher-level utilities migrated or rewritten from the old CommonUtils module: JSON, XML, crypto helpers, byte buffers, UUID generation, math helpers, and safe conversion helpers.

Existing bundled jsoncpp/tinyxml code should be replaced with nlohmann_json and pugixml. Crypto should use OpenSSL through small, testable wrappers.

### nexus_net

`nexus_net` replaces HCNetUtils. It provides TCP, UDP, HTTP, and WebSocket abstractions. The API should be designed around explicit connection/session objects, clear lifecycle methods, callbacks for async events, and RAII cleanup.

The initial implementation should prioritize HTTP and WebSocket because the old WebSocket module depends on missing HCNetUtils.

### nexus_usb

`nexus_usb` replaces old UsbTools and the HCUSBSDK dependency. It provides USB enumeration, device descriptors, hotplug monitoring, UVC helpers, and USB audio-related discovery. Windows implementation can use SetupAPI, DirectShow, Media Foundation, WASAPI, and WinUSB where appropriate. Linux implementation can use libudev, V4L2, ALSA/PulseAudio, and system APIs.

Old USB code should be reviewed for useful device parsing logic, but platform-specific code should be rewritten where needed to improve correctness and resource management.

### nexus_hid

`nexus_hid` provides HID device enumeration and read/write operations based on hidapi. It should expose simple RAII device handles and safe buffer APIs.

### nexus_media

`nexus_media` replaces old MediaTools and media_tube. It provides demuxing, decoding, encoding, remuxing, resampling, scaling, audio mixing primitives, and format conversion based on locally built FFmpeg. Audio capture/playback integration should use PortAudio first, with optional native backends where needed.

Media processing APIs should hide FFmpeg ownership rules from consumers and expose clear C++ objects such as `Decoder`, `Encoder`, `Frame`, `Packet`, and `MediaPipeline`.

### nexus_screen

`nexus_screen` replaces SCE. It provides desktop and window capture. Windows should start with a practical implementation using GDI or Desktop Duplication depending on OS/API availability. Linux should start with X11-based capture, with a later path for PipeWire/Wayland.

## 7. Public API Style

NexusKit APIs should use:

- `namespace nexus::<module>`.
- Lowercase snake_case header names.
- RAII resource ownership.
- `std::string`, `std::vector`, `std::chrono`, `std::filesystem`, and `std::function`.
- `nexus::Result<T>` and `nexus::Status` for recoverable errors.
- Exceptions only for programmer errors or where a dependency forces them internally.
- No raw owning pointers in public APIs.
- No platform headers in public headers unless the module explicitly exposes a platform extension.

Example includes:

```cpp
#include <nexus/log/logger.h>
#include <nexus/net/websocket_client.h>
#include <nexus/hid/device.h>
#include <nexus/media/decoder.h>
#include <nexus/screen/capturer.h>
```

## 8. Error Handling

The base error model is:

- `nexus::Status` for success or failure with a code and message.
- `nexus::Result<T>` for either a value or a `Status`.
- Error codes grouped by module.
- Native platform errors preserved as optional diagnostic data.

Public APIs should avoid ambiguous boolean returns for operations that can fail for multiple reasons.

## 9. Logging

Logging is provided by `nexus_log`, not by global macros scattered across modules. Modules may accept an optional logger or use a named module logger. Log configuration should support code-based setup first; file-based config can be added later.

The old hlog macros are not preserved. Where old code is migrated, logging calls should be rewritten to the new logger API.

## 10. Threading and Async Model

The initial async model is callback-based with explicit lifecycle ownership. The network, USB hotplug, audio, and screen modules should not create uncontrolled global threads. Long-running services should be represented by objects that can be started and stopped.

C++20 coroutines are not required. The project stays C++17-compatible.

## 11. Platform Strategy

Platform-specific implementation files should be split clearly, for example:

```text
src/usb/device_monitor_win.cpp
src/usb/device_monitor_linux.cpp
src/screen/capturer_win.cpp
src/screen/capturer_x11.cpp
```

Public headers should remain platform-neutral wherever possible. CMake decides which implementation files are compiled for each platform.

Windows-specific APIs may use:

- SetupAPI and WinUSB for USB.
- DirectShow or Media Foundation for UVC discovery/capture where appropriate.
- WASAPI and PortAudio for audio.
- Desktop Duplication or GDI for screen capture.

Linux-specific APIs may use:

- libudev for device enumeration and hotplug.
- V4L2 for camera/UVC.
- ALSA/PulseAudio/PortAudio for audio.
- X11/Xrandr/Xinerama for screen capture.

## 12. Testing Strategy

The project should include unit tests and integration examples. Tests should use CTest and Catch2.

Initial test areas:

- `Status` and `Result<T>`.
- Filesystem and string helpers.
- JSON/XML parsing helpers.
- Crypto wrappers with known test vectors.
- HID enumeration smoke tests, gated by hardware availability.
- FFmpeg wrapper tests using generated media fixtures.
- Network loopback tests for HTTP/WebSocket.

Hardware-dependent tests must be opt-in.

## 13. Documentation Strategy

Documentation should follow common open-source expectations:

- `README.md` for overview, features, quick start, and build commands.
- `docs/build.md` for Windows, Linux, proxy, and third-party build notes.
- `docs/architecture.md` for module boundaries and dependency graph.
- `docs/modules/*.md` for module-specific APIs.
- `examples/` for runnable examples.
- Optional Doxygen generation after public APIs stabilize.

The documentation should explain when a dependency is built from source and when a system package is required.

## 14. Migration From Legacy Code

Legacy code is a reference, not a compatibility contract.

Useful candidates for migration:

- Byte buffers, UUID, string conversion, math, and selected common utilities.
- Lessons from old USB, HID, UVC, PulseAudio, WASAPI, and V4L2 implementations.
- FFmpeg usage patterns from MediaTools where they are correct and maintainable.

Code that should be rewritten or discarded:

- Code tied to hpr, hlog, HCNetUtils, HCUSBSDK, media_tube, and SCE.
- Global logging macros.
- Platform-specific code exposed through public headers.
- Manual resource management that can be replaced by RAII.
- Old Visual Studio, QMake, and scattered output directory conventions.
- Encoded or garbled comments that do not help maintenance.

When old code is reused, it must be reviewed for thread safety, resource ownership, encoding issues, error handling, and platform assumptions.

## 15. Implementation Phases

### Phase 1: Project Skeleton

Create the CMake root, CMake presets, standard directories, README, build documentation, and initial `nexus_core` target. Add CI-ready CTest structure even if CI itself is added later.

### Phase 2: Third-Party Build Recipes

Add reproducible build/download recipes for spdlog, hidapi, OpenSSL, PortAudio, and FFmpeg. Document proxy configuration and platform prerequisites.

### Phase 3: Core, Log, and Common

Implement `nexus_core`, `nexus_log`, and a small first slice of `nexus_common`. Add tests for result/status, logging setup, and selected utility functions.

### Phase 4: HID and USB

Implement `nexus_hid` on hidapi. Implement `nexus_usb` enumeration and hotplug for Windows and Linux. Add examples and hardware-gated tests.

### Phase 5: Network

Implement `nexus_net` for HTTP and WebSocket first, then TCP and UDP. Add loopback tests and examples.

### Phase 6: Media

Implement FFmpeg wrappers for basic demux/decode/remux and audio processing. Add generated media fixtures and tests. Integrate PortAudio for simple audio I/O.

### Phase 7: Screen Capture

Implement `nexus_screen` for Windows and Linux. Add examples for frame capture and optional integration with `nexus_media`.

### Phase 8: Packaging and Polish

Finalize install/export targets, examples, documentation, and distribution layout. Remove obsolete legacy project files only after the new project is usable.

## 16. Phase 1 Skeleton Acceptance Criteria

The Phase 1 skeleton is complete when:

- CMake can configure the new project on Windows with MSVC 2022.
- CMake presets are present for Windows/MSVC, Windows/Ninja, and Linux/Ninja.
- `nexus_core` builds successfully.
- The first Catch2-backed tests run through CTest.
- Build outputs are under `build/`.
- Install/export targets generate a consumable `NexusKitConfig.cmake`.
- The project has README, build docs, architecture docs, and `nexus_core` module docs.

`nexus_log` and broader dependency-backed modules belong to later implementation phases.
