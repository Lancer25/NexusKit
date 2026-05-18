# nexus_screen

`nexus_screen` contains desktop and window capture utilities. It currently provides primary-display, display-id, region, and visible-window capture for BGRA screen frames.

## Current API

- `nexus::screen::ScreenBackendInfo`: backend availability and description.
- `nexus::screen::screen_backend_info`: returns the current screen backend summary.
- `nexus::screen::ScreenPixelFormat`: frame pixel format such as BGRA or RGBA.
- `nexus::screen::screen_pixel_format_name`: returns canonical string names for public pixel format enum values.
- `nexus::screen::ScreenFrame`: captured frame width, height, timestamp, pixel format, and bytes.
- `nexus::screen::ScreenDisplay`: display id, name, desktop origin, dimensions, and primary flag.
- `nexus::screen::ScreenWindow`: capturable visible-window id, containing display id, process id, active flag, title, desktop origin, and dimensions.
- `nexus::screen::ScreenCaptureRegion`: display-relative rectangle for region capture.
- `nexus::screen::ScreenCaptureOptions`: capture options such as cursor inclusion.
- `nexus::screen::write_ppm_file`: write a BGRA screen frame to a binary P6 PPM image.
- `nexus::screen::ScreenCapturer`: move-only RAII capture facade with primary-display, display-id, region, and visible-window capture.

## Scope

On Windows, the module uses DXGI Desktop Duplication through a private D3D11 implementation. `ScreenCapturer::displays` enumerates attached DXGI outputs, `capture_primary` captures the primary display, and `capture_display` captures a selected display id. Captured frames are contiguous BGRA (`ScreenPixelFormat::bgra`) with `data.size() == width * height * 4`.

`ScreenCapturer::capture_region` captures a full primary or display-id frame through the active backend, then returns a display-relative rectangular crop. Leave `ScreenCaptureRegion::display_id` empty to crop the primary display, or set it to a value returned by `displays` to crop a specific display. The region origin must be non-negative, dimensions must be positive, and the rectangle must fit inside the source frame.

`ScreenCapturer::windows` enumerates currently capturable visible windows where supported. On Windows, each returned window includes the `display_id` of the containing display, the owning process id, and an `active` flag for the foreground window when it is capturable. `capture_window` uses DXGI Desktop Duplication and crops the window rectangle out of that display frame. It returns the window as currently visible on the desktop, so occluding windows and compositor effects are included. Minimized, untitled, off-display, and cross-display spanning windows are not listed. Window ids are opaque and should not be persisted across runs.

Set `ScreenCaptureOptions::include_cursor = true` to request best-effort cursor composition on Windows. Cursor query or bitmap extraction failures do not fail the screen capture; the backend returns the desktop frame without cursor overlay in those cases.

On Linux, the module uses X11 when system X11 development files are available at configure time. When libXrandr development headers are also present, `displays()` enumerates connected monitor outputs with per-monitor names, positions, dimensions, and primary flags. `capture_display("xrandr-<name>")` crops the matching monitor area from the root window. When Xrandr is unavailable at build time, the backend falls back to a single `x11-default` display for the entire root window. Linux X11 window enumeration and capture use `XQueryTree`-based visible-window discovery with EWMH `_NET_WM_NAME`, `_NET_WM_PID`, and `_NET_ACTIVE_WINDOW` metadata, and root-window-crop capture consistent with the Windows DXGI visible-pixel semantics. Windows are assigned to the display containing their center point when per-monitor display data is available. Headless Linux sessions or builds without X11 development files still use the unavailable fallback backend.

Linux cursor composition, XShm acceleration, Wayland, and PipeWire remain future work.

Windows sessions where Desktop Duplication is not supported, such as some remote desktop, locked desktop, permission-restricted, or unsupported GPU/session states, may report unavailable at capturer creation time.

Planned follow-up work includes occlusion-free/minimized window capture, richer Linux monitor enumeration, Linux cursor composition, XShm acceleration, and possible PipeWire/Wayland support later.

All public types and functions are annotated with Doxygen `///` comments following `docs/api-style.md`.

## Example

When `NEXUS_BUILD_EXAMPLES=ON` and `NEXUS_ENABLE_SCREEN=ON`, NexusKit builds `nexus_example_screen_capture`.

```powershell
nexus_example_screen_capture --list-displays
nexus_example_screen_capture --list-windows
nexus_example_screen_capture screen_capture.ppm --display dxgi-output-0 --cursor
nexus_example_screen_capture window.ppm --window win32-hwnd-123456
nexus_example_screen_capture region.ppm --display dxgi-output-0 --region 100,100,640,360
```

The example lists displays/windows or captures one frame and writes a binary P6 PPM image through `nexus::screen::write_ppm_file`. Window listing prints the window id, containing display id, process id, optional active marker, title, and bounds. The `--cursor` flag requests cursor overlay on platforms that support it. The `--region` flag captures a display-relative rectangle as `x,y,width,height`; `--window` captures a listed window id.

`screen_pixel_format_name` returns the canonical names used in docs and tests for public `ScreenPixelFormat` enum values. `write_ppm_file` is intended as a lightweight diagnostics and test artifact helper. It accepts BGRA `ScreenFrame` data and drops alpha while writing RGB bytes.

## Error Model

- Builds without a screen backend return `StatusCode::kFailedPrecondition` from capture calls.
- Invalid region coordinates return `StatusCode::kInvalidArgument` before backend capture when possible.
- Unsupported window enumeration or capture returns `StatusCode::kFailedPrecondition`.
- Windows builds with a DXGI backend can still return `StatusCode::kFailedPrecondition` when Desktop Duplication is unavailable in the current session.
- Linux X11 builds can return `StatusCode::kFailedPrecondition` when no X display is available in the current session.
- `capture_primary` returns `StatusCode::kUnavailable` when the backend exists but no new frame is produced before the short capture timeout.
- Unexpected D3D11/DXGI failures after setup return `StatusCode::kInternal`.
- `close` is idempotent and returns OK.
- Platform handles and Windows headers remain private implementation details and are not exposed through public headers.
