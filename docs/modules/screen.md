# nexus_screen

`nexus_screen` contains desktop and window capture utilities. Phase 7B adds the first real backend: Windows DXGI 1.2 Desktop Duplication for primary-display capture.

## Current API

- `nexus::screen::ScreenBackendInfo`: backend availability and description.
- `nexus::screen::screen_backend_info`: returns the current screen backend summary.
- `nexus::screen::ScreenPixelFormat`: frame pixel format such as BGRA or RGBA.
- `nexus::screen::ScreenFrame`: captured frame width, height, timestamp, pixel format, and bytes.
- `nexus::screen::ScreenCaptureOptions`: capture options such as cursor inclusion.
- `nexus::screen::ScreenCapturer`: move-only RAII capture facade.

## Scope

On Windows, the module uses DXGI Desktop Duplication through a private D3D11 implementation. `ScreenCapturer::capture_primary` captures the primary display and returns contiguous BGRA frames (`ScreenPixelFormat::bgra`) with `data.size() == width * height * 4`.

This phase intentionally does not composite the cursor, select among multiple displays, or capture individual windows. `ScreenCaptureOptions::include_cursor` is accepted for API stability but cursor overlay remains future work.

Non-Windows builds still provide the platform-neutral facade and report an unavailable backend. Windows sessions where Desktop Duplication is not supported, such as some remote desktop, locked desktop, permission-restricted, or unsupported GPU/session states, may also report unavailable at capturer creation time.

Planned follow-up work includes cursor composition, multi-monitor selection, window capture, Linux X11 capture, and possible PipeWire/Wayland support later.

## Error Model

- Builds without a screen backend return `StatusCode::kFailedPrecondition` from capture calls.
- Windows builds with a DXGI backend can still return `StatusCode::kFailedPrecondition` when Desktop Duplication is unavailable in the current session.
- `capture_primary` returns `StatusCode::kUnavailable` when the backend exists but no new frame is produced before the short capture timeout.
- Unexpected D3D11/DXGI failures after setup return `StatusCode::kInternal`.
- `close` is idempotent and returns OK.
- Platform handles and Windows headers remain private implementation details and are not exposed through public headers.
