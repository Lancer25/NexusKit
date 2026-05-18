#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/screen/export.h>

/// Desktop and window capture.
///
/// On Windows the backend uses DXGI Desktop Duplication via a private D3D11
/// implementation.  On Linux the backend uses X11 when X11 development headers
/// are available at configure time.  Builds without a supported backend report
/// `available == false` and capture calls return `kFailedPrecondition`.
///
/// Public headers do not expose platform or graphics API types.
namespace nexus::screen {

namespace detail {
class ScreenCapturerStorage;
}

/// Screen capture backend build and runtime information.
struct ScreenBackendInfo {
    /// Backend identifier (e.g. "dxgi", "x11", "unavailable").
    std::string name = "screen";
    /// True when a platform backend is available at runtime.
    bool available = false;
    /// Human-readable backend description.
    std::string description;
};

/// Screen frame pixel formats.
enum class ScreenPixelFormat {
    unknown,
    /// BGRA 8-bit, 4 bytes per pixel, rows are contiguous.
    bgra,
    /// RGBA 8-bit, 4 bytes per pixel, rows are contiguous.
    rgba
};

/// A captured screen frame.
///
/// Frame data is a contiguous pixel buffer.  For BGRA and RGBA formats,
/// `data.size() == width * height * 4`.
struct ScreenFrame {
    /// Frame width in pixels.
    int width = 0;
    /// Frame height in pixels.
    int height = 0;
    /// Pixel format of `data`.
    ScreenPixelFormat pixel_format = ScreenPixelFormat::unknown;
    /// Monotonic capture timestamp in milliseconds.
    std::int64_t timestamp_ms = 0;
    /// Pixel data.  Layout depends on `pixel_format`.
    std::vector<std::uint8_t> data;
};

/// A logical display (monitor).
struct ScreenDisplay {
    /// Opaque backend id.  Do not persist across runs.
    std::string id;
    /// Platform display name (e.g. DXGI output name, or "x11-default").
    std::string name;
    /// Desktop x-coordinate of the display origin.
    int x = 0;
    /// Desktop y-coordinate of the display origin.
    int y = 0;
    /// Display width in desktop coordinates.
    int width = 0;
    /// Display height in desktop coordinates.
    int height = 0;
    /// True when this is the primary display.
    bool primary = false;
};

/// A capturable visible desktop window.
///
/// Only top-level, visible, on-display windows are returned.  Minimized,
/// hidden, off-screen, and cross-display spanning windows are excluded.
/// Window ids are opaque and should not be persisted across runs.
///
/// On Linux, `process_id` is always 0.
struct ScreenWindow {
    /// Opaque backend id.  Do not persist across runs.
    std::string id;
    /// The containing display id, or empty when unknown.
    std::string display_id;
    /// Owning process id.  0 on Linux or when unavailable.
    std::uint32_t process_id = 0;
    /// Window title (UTF-8).
    std::string title;
    /// True when this window is the foreground/active window.
    bool active = false;
    /// Desktop x-coordinate of the window rectangle.
    int x = 0;
    /// Desktop y-coordinate of the window rectangle.
    int y = 0;
    /// Window rectangle width.
    int width = 0;
    /// Window rectangle height.
    int height = 0;
};

/// Display-relative rectangle for `capture_region`.
///
/// Leave `display_id` empty to target the primary display, or set it to a
/// value from `displays()`.
struct ScreenCaptureRegion {
    /// Target display id.  Empty means primary display.
    std::string display_id;
    /// Left offset within the display.
    int x = 0;
    /// Top offset within the display.
    int y = 0;
    /// Crop width.  Must be positive.
    int width = 0;
    /// Crop height.  Must be positive.
    int height = 0;
};

/// Capture flags passed to `ScreenCapturer::create`.
struct ScreenCaptureOptions {
    /// When true, request best-effort cursor composition on Windows.
    /// Failure to query or render the cursor does not fail the capture.
    bool include_cursor = false;
};

/// Returns the canonical name for a `ScreenPixelFormat` value (e.g. "bgra").
NEXUS_SCREEN_API std::string screen_pixel_format_name(ScreenPixelFormat format);

/// Returns screen capture backend build/runtime information.
NEXUS_SCREEN_API ScreenBackendInfo screen_backend_info();

/// Writes a BGRA `ScreenFrame` to a binary P6 PPM image file.
///
/// Alpha is dropped; only RGB bytes are written.  This is a lightweight
/// inspection and test utility.
///
/// @return `kInvalidArgument` when the frame format is not BGRA.
/// `kInternal` on file write errors.
NEXUS_SCREEN_API Status write_ppm_file(
    const std::filesystem::path& path,
    const ScreenFrame& frame);

/// Move-only RAII screen capture facade.
///
/// Created via `create(options)`.  Copy is deleted; move transfers ownership.
/// A moved-from capturer is closed and `is_available()` returns false.
///
/// On Windows, `create` may fail (`kFailedPrecondition`) when Desktop
/// Duplication is unavailable in the current session (remote desktop, locked
/// desktop, restricted GPU state).
///
/// On Linux, `create` may fail (`kFailedPrecondition`) when no X display is
/// available.
///
/// Capture methods may block briefly while waiting for a new frame.
///
/// Intended for single-threaded use.
class NEXUS_SCREEN_API ScreenCapturer {
public:
    /// Constructs a closed capturer.
    ScreenCapturer();
    ScreenCapturer(const ScreenCapturer&) = delete;
    ScreenCapturer& operator=(const ScreenCapturer&) = delete;
    ScreenCapturer(ScreenCapturer&& other) noexcept;
    ScreenCapturer& operator=(ScreenCapturer&& other) noexcept;
    /// Closes the capturer if open.
    ~ScreenCapturer();

    /// Creates a screen capturer for the current session.
    ///
    /// @return An available capturer on success.
    /// @retval kFailedPrecondition when no platform backend is available or
    /// the backend cannot initialize (e.g. DXGI unavailable, no X display).
    static Result<ScreenCapturer> create(const ScreenCaptureOptions& options = {});

    /// True when the capturer is open and the backend is initialized.
    bool is_available() const;

    /// Enumerates attached displays.
    ///
    /// On Windows, enumerates DXGI outputs.  On Linux with X11, returns a
    /// single "x11-default" display for the default root window.
    ///
    /// @return Display list on success.
    /// @retval kFailedPrecondition when the capturer is closed or the backend
    /// is unavailable.
    Result<std::vector<ScreenDisplay>> displays() const;

    /// Enumerates capturable visible desktop windows.
    ///
    /// On Windows, uses `EnumWindows`-based discovery with visibility checks.
    /// On Linux with X11, uses `XQueryTree` with EWMH metadata.
    ///
    /// @return Window list on success.
    /// @retval kFailedPrecondition when the capturer is closed, the backend is
    /// unavailable, or window enumeration is not supported on this platform.
    Result<std::vector<ScreenWindow>> windows() const;

    /// Captures the primary display.
    ///
    /// Waits briefly for a new frame.
    ///
    /// @return A BGRA frame on success.
    /// @retval kFailedPrecondition when the capturer is closed or the backend
    /// is unavailable.
    /// @retval kUnavailable when no new frame is produced before the internal
    /// timeout.
    Result<ScreenFrame> capture_primary();

    /// Captures a display by id.
    ///
    /// The id must come from a previous `displays()` call in the same session.
    ///
    /// @return A BGRA frame on success.
    /// @retval kInvalidArgument when `display_id` is empty or unknown.
    /// @retval kFailedPrecondition when the capturer is closed or the backend
    /// is unavailable.
    /// @retval kUnavailable when no new frame is produced before the internal
    /// timeout.
    Result<ScreenFrame> capture_display(const std::string& display_id);

    /// Captures a display-relative region.
    ///
    /// Captures the full frame from the target display, then crops.
    /// The region origin must be non-negative, dimensions positive, and the
    /// rectangle must fit inside the source frame.
    ///
    /// @return A BGRA crop on success.
    /// @retval kInvalidArgument when the region is out of bounds.
    /// @retval kFailedPrecondition when the capturer is closed or the backend
    /// is unavailable.
    Result<ScreenFrame> capture_region(const ScreenCaptureRegion& region);

    /// Captures the visible pixel rectangle of a desktop window.
    ///
    /// The window id must come from a previous `windows()` call in the same
    /// session.  The returned frame contains the window as currently visible
    /// on the desktop; occluding windows and compositor effects are included.
    ///
    /// On Windows, uses DXGI Desktop Duplication and crops the window rect
    /// from the containing display frame.
    /// On Linux with X11, uses root-window crop.
    ///
    /// @return A BGRA frame of the window's visible area on success.
    /// @retval kInvalidArgument when `window_id` is empty or unknown.
    /// @retval kFailedPrecondition when the capturer is closed or the backend
    /// is unavailable.
    Result<ScreenFrame> capture_window(const std::string& window_id);

    /// Closes the capturer.  Idempotent; safe to call on a closed capturer.
    Status close();

private:
    explicit ScreenCapturer(std::unique_ptr<detail::ScreenCapturerStorage> storage);

    std::unique_ptr<detail::ScreenCapturerStorage> storage_;
};

} // namespace nexus::screen
