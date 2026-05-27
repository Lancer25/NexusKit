# Iteration Guide

This guide records how future NexusKit sessions should choose and execute work.

## Default Workflow

Each implementation phase should be small, testable, and documented.

1. Inspect `git status`, recent `git log`, the relevant public headers, implementation files, tests, and module docs.
2. Add a design spec under `docs/superpowers/specs`.
3. Add an implementation plan under `docs/superpowers/plans`.
4. Write a failing test or documentation check first.
5. Implement the smallest production change that satisfies the test.
6. Update module docs, README, architecture docs, build docs, and changelog when relevant.
7. Run focused tests.
8. Run a fresh focused configure/build/test/install verification when behavior or build surface changes.
9. Commit and push to `master` unless the active task says otherwise.

## Project Rules

- Continue directly on `master` unless the user requests a branch.
- Do not revert unrelated local changes.
- Public headers must stay free of backend-private platform and third-party headers.
- New APIs must include public header comments following `docs/api-style.md`.
- Prefer existing module patterns and `nexus_common` helpers before adding local duplicates.
- Keep feature work conservative: one natural stage per commit.
- Use `nexus::Status` and `nexus::Result<T>` for recoverable failures.
- Keep diagnostics routed through `nexus_log` or `nexus_common` logging helpers.

## Completed Recent Work

- Phase 8D: Doxygen API comments for `nexus_core` and `nexus_common`.
- Phase 8E: Doxygen API comments for `nexus_media` and `nexus_screen`.
- Phase 8F: Doxygen API comments for `nexus_net`, `nexus_usb`, `nexus_hid`, and `nexus_log`.
- Phase 8G/8H: TCP connect/read/write timeout options.
- Phase 8I: Explicit media stream-index selection.
- Phase 8J: Decoder EOF flushing.
- Phase 8K: Linux Xrandr display enumeration.
- Phase 8L: HID report id and feature report documentation.
- Phase 8M: USB hotplug monitor and common threading utilities.
- Phase 9A: Media encoder.
- Phase 9B: Media muxer.
- Phase 9C: Repository hygiene, roadmap cleanup, and newly added media docs/spec tracking.
- Phase 9D: Media encoder and muxer timestamp semantics.
- Phase 9E: USB hotplug callback validation and stop semantics.
- Phase 9F: USB hotplug listing/watch example.
- Phase 10A: UDP receive timeout and WebSocket write timeout.
- Phase 9G: Media video encoding test and mixed audio/video muxing test.
- Phase 9H: Screen multi-display window-to-display mapping validation and stale Xrandr roadmap cleanup.
- Phase 10B: Async TCP and UDP transport wrappers (callback-driven, private Asio io_context workers).
- Phase 11A: Merge async HTTP methods into `HttpClient`, removing separate `AsyncHttpClient`.
- Phase 11B: Add async methods to `WebSocketClient`.
- Phase 11C: TLS support for `HttpClient` (https://) and `WebSocketClient` (wss://).
- Phase 11D: Merge `AsyncUdpSocket` into `UdpSocket` for unified sync/async UDP.
- Phase 11E: Add `WebSocketServer` with multi-client support, sync/async listen, and lifecycle callbacks.
- Phase 11F: Add OpenSSL DLL delay-load on MSVC for optional TLS at runtime.
- Phase 12A: Add `nexus_audio` with WASAPI backend - audio input device enumeration and PCM int16 capture.
- Phase 12B: Add `nexus_camera` with libuvc (Windows) and V4L2 (Linux) backends - USB camera device enumeration and video frame capture.
- Phase 12C: Optimize `nexus_audio` and `nexus_camera` - add WASAPI volume/mute control, implement full PulseAudio backend, add V4L2 camera controls (brightness, contrast, etc.).
- Phase 13C-13D: Public header documentation and contract checks for callback typedefs, enum values, lifecycle methods, and ASCII dash punctuation.
- Phase 13E-13K: Repository text and documentation hygiene checks for UTF-8 policy, final newlines, trailing whitespace, UTF-8 BOMs, CI docs test discovery, `.gitattributes`, policy files, and `.gitignore`.
- Phase 14A: Public API contract checks for net callback threading, zero timeout fields, and move-only ownership semantics.

## Near-Term Roadmap

### Post-0.1.0 Roadmap

- Phase 13A: Add a CI/build matrix and packaging checks for default, optional-module, and release configurations.
- Phase 13B: Fill example coverage for modules with stable public APIs, starting with media mux/remux workflows and audio/camera smoke examples.
- Phase 14B: Extend public API contract checks for public error and precondition semantics beyond ownership, threading, and timeout coverage.

### Media Follow-Ups

- ~~Add focused muxing tests for mixed audio/video interleaving once video encode coverage is expanded.~~ Phase 9G: added `make_test_video_frame` helper, H264 video encoder test, and mixed AAC+H264 MP4 muxing test.
- ~~Consider direct container-to-container remux as a separate high-level workflow after encoder/muxer packet contracts are stable.~~ Implemented as `remux_file`.

### Screen Follow-Ups

- ~~Add Linux Xrandr display enumeration when available.~~ Phase 8K: implemented Xrandr-based display enumeration via `displays()`.
- ~~Clarify and test multi-display window behavior on Linux.~~ Phase 9H: added cross-platform multi-display window-to-display mapping validation test; each window's `display_id` must reference a valid enumerated display.
- Keep occlusion-free or minimized-window capture as a separate future design because current capture returns desktop-visible pixels.

### Net Follow-Ups

- ~~Add async transport wrappers (callback-driven, private Asio io_context workers).~~ Phase 10B-11E: all net types (`TcpClient`, `UdpSocket`, `HttpClient`, `WebSocketClient`, `WebSocketServer`) now provide async methods alongside sync.
- ~~Extend timeout option patterns to UDP, WebSocket, and HTTP.~~ Phase 10A: added `UdpReceiveOptions` and `WebSocketClientOptions::write_timeout`.
- ~~Add TLS support for HTTP (https://) and WebSocket (wss://).~~ Phase 11C: conditional OpenSSL linkage via `NEXUS_NET_HAS_TLS`.
- ~~Add HTTP redirect support.~~ Added `follow_redirects` to `HttpClientOptions`.
- Add HTTP streaming support.

### USB/HID Follow-Ups

- Do not add HID-level hotplug support unless the product requirement changes;
  USB-level `UsbHotplugMonitor` is the supported hotplug surface.

## Fresh Verification Naming

Use focused build directories that describe the phase, such as:

```text
build/phase8d-core-common-comments
build/phase8e-media-screen-comments
build/phase9d-media-timestamps
build/phase9e-usb-hotplug-lifecycle
build/phase9f-usb-hotplug-example
build/phase9g-media-mixed-muxing
build/phase9h-screen-multi-display
```

Fresh verification should avoid relying on previous build artifacts when the public API, CMake surface, or backend integration changes.
