# nexus_screen

`nexus_screen` contains desktop and window capture utilities. Phase 7A establishes the module boundary and public capture facade before platform backends are added.

## Current API

- `nexus::screen::ScreenBackendInfo`: backend availability and description.
- `nexus::screen::screen_backend_info`: returns the current screen backend summary.
- `nexus::screen::ScreenPixelFormat`: frame pixel format such as BGRA or RGBA.
- `nexus::screen::ScreenFrame`: captured frame width, height, timestamp, pixel format, and bytes.
- `nexus::screen::ScreenCaptureOptions`: capture options such as cursor inclusion.
- `nexus::screen::ScreenCapturer`: move-only RAII capture facade.

## Scope

The current module does not capture real desktop pixels yet. `ScreenCapturer::create` returns a valid facade, `is_available` reports false, and `capture_primary` returns `StatusCode::kFailedPrecondition` until a platform backend is implemented.

Planned backends include Windows GDI or Desktop Duplication, and Linux X11 first with possible PipeWire/Wayland support later.

## Error Model

- Builds without a screen backend return `StatusCode::kFailedPrecondition` from capture calls.
- `close` is idempotent and returns OK.
- Future platform failures should use `Status` or `Result<T>` and keep platform handles out of public headers.
