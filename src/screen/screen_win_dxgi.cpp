#include <nexus/common/string.h>
#include <nexus/common/time.h>

#include "screen_private.h"
#include "screen_cursor_overlay.h"
#include "screen_frame_crop.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace nexus::screen::detail {
namespace {

using Microsoft::WRL::ComPtr;

constexpr UINT kAcquireTimeoutMs = 500;

Status failed_precondition(std::string message) {
    return Status(StatusCode::kFailedPrecondition, std::move(message));
}

Status unavailable(std::string message) {
    return Status(StatusCode::kUnavailable, std::move(message));
}

Status internal(std::string message) {
    return Status(StatusCode::kInternal, std::move(message));
}

std::string window_id_from_handle(const HWND hwnd) {
    return "win32-hwnd-" + std::to_string(reinterpret_cast<std::uintptr_t>(hwnd));
}

std::int64_t qpc_timestamp_ms(const LARGE_INTEGER timestamp) {
    if (timestamp.QuadPart == 0) {
        return nexus::common::steady_timestamp_ms();
    }

    LARGE_INTEGER frequency{};
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart == 0) {
        return nexus::common::steady_timestamp_ms();
    }

    return static_cast<std::int64_t>((timestamp.QuadPart * 1000) / frequency.QuadPart);
}

struct OwnedIcon {
    HICON handle = nullptr;

    ~OwnedIcon() {
        if (handle) {
            DestroyIcon(handle);
        }
    }

    OwnedIcon(const OwnedIcon&) = delete;
    OwnedIcon& operator=(const OwnedIcon&) = delete;
};

struct OwnedBitmap {
    HBITMAP handle = nullptr;

    ~OwnedBitmap() {
        if (handle) {
            DeleteObject(handle);
        }
    }

    OwnedBitmap(const OwnedBitmap&) = delete;
    OwnedBitmap& operator=(const OwnedBitmap&) = delete;
};

struct OwnedScreenDc {
    HDC handle = nullptr;

    OwnedScreenDc() : handle(GetDC(nullptr)) {}

    ~OwnedScreenDc() {
        if (handle) {
            ReleaseDC(nullptr, handle);
        }
    }

    OwnedScreenDc(const OwnedScreenDc&) = delete;
    OwnedScreenDc& operator=(const OwnedScreenDc&) = delete;
};

bool read_bitmap_bgra(const HBITMAP bitmap, const int width, const int height,
                      std::vector<std::uint8_t>& pixels) {
    if (!bitmap || width <= 0 || height <= 0) {
        return false;
    }

    BITMAPINFO bitmap_info{};
    bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap_info.bmiHeader.biWidth = width;
    bitmap_info.bmiHeader.biHeight = -height;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    pixels.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u, 0u);

    OwnedScreenDc dc;
    if (!dc.handle) {
        return false;
    }

    const int rows = GetDIBits(dc.handle, bitmap, 0, static_cast<UINT>(height), pixels.data(),
                               &bitmap_info, DIB_RGB_COLORS);
    return rows == height;
}

void apply_cursor_mask_alpha(const std::vector<std::uint8_t>& mask_bgra,
                             CursorOverlayImage& cursor) {
    if (mask_bgra.size() != cursor.pixels_bgra.size()) {
        return;
    }

    for (std::size_t index = 0; index < cursor.pixels_bgra.size(); index += 4u) {
        const bool transparent = mask_bgra[index] == 255u && mask_bgra[index + 1u] == 255u &&
                                 mask_bgra[index + 2u] == 255u;
        cursor.pixels_bgra[index + 3u] = transparent ? 0u : 255u;
    }
}

std::optional<CursorOverlayImage> capture_cursor_overlay(const POINT output_origin) {
    CURSORINFO cursor_info{};
    cursor_info.cbSize = sizeof(cursor_info);
    if (!GetCursorInfo(&cursor_info) || !(cursor_info.flags & CURSOR_SHOWING) ||
        !cursor_info.hCursor) {
        return std::nullopt;
    }

    OwnedIcon cursor_icon{CopyIcon(cursor_info.hCursor)};
    if (!cursor_icon.handle) {
        return std::nullopt;
    }

    ICONINFO icon_info{};
    if (!GetIconInfo(cursor_icon.handle, &icon_info)) {
        return std::nullopt;
    }
    OwnedBitmap color_bitmap{icon_info.hbmColor};
    OwnedBitmap mask_bitmap{icon_info.hbmMask};

    if (!color_bitmap.handle) {
        return std::nullopt;
    }

    BITMAP bitmap{};
    if (GetObject(color_bitmap.handle, sizeof(bitmap), &bitmap) != sizeof(bitmap) ||
        bitmap.bmWidth <= 0 || bitmap.bmHeight <= 0) {
        return std::nullopt;
    }

    CursorOverlayImage cursor;
    cursor.width = bitmap.bmWidth;
    cursor.height = bitmap.bmHeight;
    cursor.x = cursor_info.ptScreenPos.x - static_cast<int>(icon_info.xHotspot) - output_origin.x;
    cursor.y = cursor_info.ptScreenPos.y - static_cast<int>(icon_info.yHotspot) - output_origin.y;

    if (!read_bitmap_bgra(color_bitmap.handle, cursor.width, cursor.height, cursor.pixels_bgra)) {
        return std::nullopt;
    }

    bool has_alpha = false;
    for (std::size_t index = 3u; index < cursor.pixels_bgra.size(); index += 4u) {
        if (cursor.pixels_bgra[index] != 0u) {
            has_alpha = true;
            break;
        }
    }

    if (!has_alpha) {
        std::vector<std::uint8_t> mask_bgra;
        if (read_bitmap_bgra(mask_bitmap.handle, cursor.width, cursor.height, mask_bgra)) {
            apply_cursor_mask_alpha(mask_bgra, cursor);
        } else {
            for (std::size_t index = 3u; index < cursor.pixels_bgra.size(); index += 4u) {
                cursor.pixels_bgra[index] = 255u;
            }
        }
    }

    return cursor;
}

Status map_setup_failure(const HRESULT hr, const char* operation) {
    switch (hr) {
    case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
    case DXGI_ERROR_UNSUPPORTED:
    case DXGI_ERROR_ACCESS_LOST:
    case DXGI_ERROR_SESSION_DISCONNECTED:
    case DXGI_ERROR_ACCESS_DENIED:
        return failed_precondition(std::string(operation) +
                                   " failed because desktop duplication is unavailable");
    default:
        return internal(std::string(operation) + " failed");
    }
}

Status map_capture_failure(const HRESULT hr, const char* operation) {
    switch (hr) {
    case DXGI_ERROR_WAIT_TIMEOUT:
        return unavailable("no desktop frame was available before the capture timeout");
    case DXGI_ERROR_ACCESS_LOST:
    case DXGI_ERROR_SESSION_DISCONNECTED:
    case DXGI_ERROR_ACCESS_DENIED:
    case DXGI_ERROR_INVALID_CALL:
        return failed_precondition(std::string(operation) +
                                   " failed because desktop duplication is unavailable");
    default:
        return internal(std::string(operation) + " failed");
    }
}

class DxgiScreenCapturerStorage final : public ScreenCapturerStorage {
public:
    explicit DxgiScreenCapturerStorage(ScreenCaptureOptions options)
        : options_(options) {
        setup_status_ = initialize();
    }

    bool is_available() const override {
        return setup_status_.ok() && duplication_;
    }

    Result<std::vector<ScreenDisplay>> displays() const override {
        if (!setup_status_.ok()) {
            return setup_status_;
        }
        if (displays_.empty()) {
            return failed_precondition("no DXGI outputs are attached to the desktop");
        }
        std::vector<ScreenDisplay> displays;
        displays.reserve(displays_.size());
        for (const auto& entry : displays_) {
            displays.push_back(entry.display);
        }
        return displays;
    }

    Result<std::vector<ScreenWindow>> windows() const override {
        if (!setup_status_.ok()) {
            return setup_status_;
        }
        if (!is_available()) {
            return failed_precondition("screen capture backend is not available");
        }

        auto windows = enumerate_windows();
        std::vector<ScreenWindow> capturable_windows;
        capturable_windows.reserve(windows.size());
        for (auto& window : windows) {
            if (const auto* display = find_containing_display(window)) {
                window.display_id = display->id;
                capturable_windows.push_back(std::move(window));
            }
        }

        return capturable_windows;
    }

    Result<ScreenFrame> capture_primary() override {
        if (!is_available()) {
            return setup_status_.ok()
                       ? failed_precondition("screen capture backend is not available")
                       : setup_status_;
        }

        const auto primary = find_primary_display();
        if (!primary) {
            return failed_precondition("no primary DXGI output is available");
        }

        return capture_display(primary->id);
    }

    Result<ScreenFrame> capture_display(const std::string& display_id) override {
        if (!setup_status_.ok()) {
            return setup_status_;
        }
        if (!select_output(display_id)) {
            if (!setup_status_.ok()) {
                return setup_status_;
            }
            return Status(StatusCode::kNotFound, "screen display was not found: " + display_id);
        }

        DXGI_OUTDUPL_FRAME_INFO frame_info{};
        ComPtr<IDXGIResource> desktop_resource;
        const HRESULT acquire_hr = duplication_->AcquireNextFrame(
            kAcquireTimeoutMs, &frame_info, desktop_resource.GetAddressOf());
        if (FAILED(acquire_hr)) {
            return map_capture_failure(acquire_hr, "AcquireNextFrame");
        }

        bool frame_acquired = true;
        auto release_frame = [&]() {
            if (frame_acquired) {
                duplication_->ReleaseFrame();
                frame_acquired = false;
            }
        };

        ComPtr<ID3D11Texture2D> desktop_texture;
        HRESULT hr = desktop_resource.As(&desktop_texture);
        if (FAILED(hr)) {
            release_frame();
            return internal("acquired desktop resource is not a D3D11 texture");
        }

        D3D11_TEXTURE2D_DESC texture_desc{};
        desktop_texture->GetDesc(&texture_desc);
        if (texture_desc.Format != DXGI_FORMAT_B8G8R8A8_UNORM) {
            release_frame();
            return failed_precondition("desktop frame is not BGRA8 format");
        }

        D3D11_TEXTURE2D_DESC staging_desc = texture_desc;
        staging_desc.BindFlags = 0;
        staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        staging_desc.MiscFlags = 0;
        staging_desc.Usage = D3D11_USAGE_STAGING;

        ComPtr<ID3D11Texture2D> staging_texture;
        hr = device_->CreateTexture2D(&staging_desc, nullptr, staging_texture.GetAddressOf());
        if (FAILED(hr)) {
            release_frame();
            return internal("failed to create a CPU-readable desktop frame texture");
        }

        context_->CopyResource(staging_texture.Get(), desktop_texture.Get());

        D3D11_MAPPED_SUBRESOURCE mapped{};
        hr = context_->Map(staging_texture.Get(), 0, D3D11_MAP_READ, 0, &mapped);
        if (FAILED(hr)) {
            release_frame();
            return map_capture_failure(hr, "Map");
        }

        ScreenFrame frame;
        frame.width = static_cast<int>(texture_desc.Width);
        frame.height = static_cast<int>(texture_desc.Height);
        frame.pixel_format = ScreenPixelFormat::bgra;
        frame.timestamp_ms = qpc_timestamp_ms(frame_info.LastPresentTime);

        const std::size_t row_bytes = static_cast<std::size_t>(frame.width) * 4u;
        frame.data.resize(row_bytes * static_cast<std::size_t>(frame.height));

        const auto* source = static_cast<const std::uint8_t*>(mapped.pData);
        auto* destination = frame.data.data();
        for (int y = 0; y < frame.height; ++y) {
            std::memcpy(destination + (static_cast<std::size_t>(y) * row_bytes),
                        source + (static_cast<std::size_t>(y) * mapped.RowPitch),
                        row_bytes);
        }

        context_->Unmap(staging_texture.Get(), 0);
        release_frame();

        if (options_.include_cursor) {
            if (auto cursor = capture_cursor_overlay(output_origin_)) {
                overlay_cursor_bgra(frame, *cursor);
            }
        }

        return frame;
    }

    Result<ScreenFrame> capture_window(const std::string& window_id) override {
        if (window_id.empty()) {
            return Status::invalid_argument("screen window id cannot be empty");
        }
        if (!is_available()) {
            return setup_status_.ok()
                       ? failed_precondition("screen capture backend is not available")
                       : setup_status_;
        }

        const auto windows_result = windows();
        if (!windows_result.ok()) {
            return windows_result.status();
        }

        for (const auto& window : windows_result.value()) {
            if (window.id != window_id) {
                continue;
            }

            ScreenDisplay display;
            if (window.display_id.empty()) {
                const auto* containing_display = find_containing_display(window);
                if (!containing_display) {
                    return failed_precondition(
                        "screen window is not fully contained in one display");
                }
                display = *containing_display;
            } else {
                const auto* display_entry = find_display_entry(window.display_id);
                if (!display_entry) {
                    return Status(StatusCode::kNotFound,
                                  "screen display was not found: " + window.display_id);
                }
                display = display_entry->display;
            }

            ScreenCaptureRegion region;
            region.display_id = display.id;
            region.x = window.x - display.x;
            region.y = window.y - display.y;
            region.width = window.width;
            region.height = window.height;

            auto source = capture_display(region.display_id);
            if (!source.ok()) {
                return source.status();
            }

            return crop_frame_bgra(source.value(), region);
        }

        return Status(StatusCode::kNotFound, "screen window was not found: " + window_id);
    }

    void close() override {
        duplication_.Reset();
        output_.Reset();
        adapter_.Reset();
        dxgi_device_.Reset();
        context_.Reset();
        device_.Reset();
        setup_status_ = failed_precondition("screen capture backend is closed");
    }

private:
    struct DisplayEntry {
        ScreenDisplay display;
        DXGI_OUTPUT_DESC desc{};
        ComPtr<IDXGIOutput1> output;
    };

    static BOOL CALLBACK enumerate_window_proc(HWND hwnd, LPARAM parameter) {
        auto* windows = reinterpret_cast<std::vector<ScreenWindow>*>(parameter);
        if (!windows || !IsWindowVisible(hwnd) || IsIconic(hwnd)) {
            return TRUE;
        }

        const int title_length = GetWindowTextLengthW(hwnd);
        if (title_length <= 0) {
            return TRUE;
        }

        std::wstring title(static_cast<std::size_t>(title_length) + 1u, L'\0');
        const int copied = GetWindowTextW(hwnd, title.data(), title_length + 1);
        if (copied <= 0) {
            return TRUE;
        }
        title.resize(static_cast<std::size_t>(copied));

        RECT rect{};
        if (!GetWindowRect(hwnd, &rect)) {
            return TRUE;
        }

        const int width = rect.right - rect.left;
        const int height = rect.bottom - rect.top;
        if (width <= 0 || height <= 0) {
            return TRUE;
        }

        DWORD process_id = 0;
        GetWindowThreadProcessId(hwnd, &process_id);
        if (process_id == 0) {
            return TRUE;
        }

        ScreenWindow window;
        window.id = window_id_from_handle(hwnd);
        window.title = nexus::common::wide_to_utf8(title);
        window.process_id = static_cast<std::uint32_t>(process_id);
        window.active = hwnd == GetForegroundWindow();
        window.x = rect.left;
        window.y = rect.top;
        window.width = width;
        window.height = height;
        windows->push_back(std::move(window));
        return TRUE;
    }

    std::vector<ScreenWindow> enumerate_windows() const {
        std::vector<ScreenWindow> windows;
        EnumWindows(enumerate_window_proc, reinterpret_cast<LPARAM>(&windows));
        return windows;
    }

    const ScreenDisplay* find_containing_display(const ScreenWindow& window) const {
        const auto window_left = static_cast<long>(window.x);
        const auto window_top = static_cast<long>(window.y);
        const auto window_right = window_left + static_cast<long>(window.width);
        const auto window_bottom = window_top + static_cast<long>(window.height);

        for (const auto& entry : displays_) {
            const auto display_left = static_cast<long>(entry.display.x);
            const auto display_top = static_cast<long>(entry.display.y);
            const auto display_right = display_left + static_cast<long>(entry.display.width);
            const auto display_bottom = display_top + static_cast<long>(entry.display.height);

            if (window_left >= display_left && window_top >= display_top &&
                window_right <= display_right && window_bottom <= display_bottom) {
                return &entry.display;
            }
        }

        return nullptr;
    }

    Status initialize() {
        UINT create_flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
        D3D_FEATURE_LEVEL feature_levels[] = {D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
                                              D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_10_0};
        D3D_FEATURE_LEVEL selected_feature_level = D3D_FEATURE_LEVEL_11_0;

        HRESULT hr = D3D11CreateDevice(nullptr,
                                       D3D_DRIVER_TYPE_HARDWARE,
                                       nullptr,
                                       create_flags,
                                       feature_levels,
                                       static_cast<UINT>(std::size(feature_levels)),
                                       D3D11_SDK_VERSION,
                                       device_.GetAddressOf(),
                                       &selected_feature_level,
                                       context_.GetAddressOf());
        if (FAILED(hr)) {
            return map_setup_failure(hr, "D3D11CreateDevice");
        }

        hr = device_.As(&dxgi_device_);
        if (FAILED(hr)) {
            return internal("failed to query IDXGIDevice from D3D11 device");
        }

        hr = dxgi_device_->GetAdapter(adapter_.GetAddressOf());
        if (FAILED(hr)) {
            return internal("failed to query DXGI adapter from D3D11 device");
        }

        Status display_status = enumerate_outputs();
        if (!display_status.ok()) {
            return display_status;
        }

        const auto primary = find_primary_display();
        if (!primary) {
            return failed_precondition("no primary DXGI output is available");
        }

        if (!select_output(primary->id)) {
            return failed_precondition("failed to initialize the primary DXGI output");
        }

        return Status::ok_status();
    }

    Status enumerate_outputs() {
        displays_.clear();

        for (UINT index = 0;; ++index) {
            ComPtr<IDXGIOutput> output;
            const HRESULT hr = adapter_->EnumOutputs(index, output.GetAddressOf());
            if (hr == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            if (FAILED(hr)) {
                return map_setup_failure(hr, "EnumOutputs");
            }

            DXGI_OUTPUT_DESC desc{};
            const HRESULT desc_hr = output->GetDesc(&desc);
            if (FAILED(desc_hr)) {
                return internal("failed to query DXGI output description");
            }
            if (!desc.AttachedToDesktop) {
                continue;
            }

            ComPtr<IDXGIOutput1> output1;
            const HRESULT output_hr = output.As(&output1);
            if (FAILED(output_hr)) {
                continue;
            }

            ComPtr<IDXGIOutputDuplication> probe_duplication;
            const HRESULT duplication_hr =
                output1->DuplicateOutput(device_.Get(), probe_duplication.GetAddressOf());
            if (FAILED(duplication_hr)) {
                return map_setup_failure(duplication_hr, "DuplicateOutput");
            }
            DXGI_OUTDUPL_DESC duplication_desc{};
            probe_duplication->GetDesc(&duplication_desc);

            DisplayEntry entry;
            entry.desc = desc;
            entry.output = output1;
            entry.display.id = "dxgi-output-" + std::to_string(index);
            entry.display.name = nexus::common::wide_to_utf8(desc.DeviceName);
            entry.display.x = desc.DesktopCoordinates.left;
            entry.display.y = desc.DesktopCoordinates.top;
            entry.display.width = static_cast<int>(duplication_desc.ModeDesc.Width);
            entry.display.height = static_cast<int>(duplication_desc.ModeDesc.Height);
            entry.display.primary = entry.display.x == 0 && entry.display.y == 0;
            displays_.push_back(std::move(entry));
        }

        if (displays_.empty()) {
            return failed_precondition("no DXGI outputs are attached to the desktop");
        }

        bool has_primary = false;
        for (const auto& entry : displays_) {
            if (entry.display.primary) {
                has_primary = true;
                break;
            }
        }
        if (!has_primary) {
            displays_.front().display.primary = true;
        }

        return Status::ok_status();
    }

    const ScreenDisplay* find_primary_display() const {
        for (const auto& entry : displays_) {
            if (entry.display.primary) {
                return &entry.display;
            }
        }
        return nullptr;
    }

    DisplayEntry* find_display_entry(const std::string& display_id) {
        for (auto& entry : displays_) {
            if (entry.display.id == display_id) {
                return &entry;
            }
        }
        return nullptr;
    }

    bool select_output(const std::string& display_id) {
        if (current_display_id_ == display_id && duplication_) {
            return true;
        }

        auto* entry = find_display_entry(display_id);
        if (!entry || !entry->output) {
            return false;
        }

        ComPtr<IDXGIOutputDuplication> duplication;
        const HRESULT hr = entry->output->DuplicateOutput(device_.Get(), duplication.GetAddressOf());
        if (FAILED(hr)) {
            setup_status_ = map_setup_failure(hr, "DuplicateOutput");
            duplication_.Reset();
            return false;
        }

        output_ = entry->output;
        duplication_ = duplication;
        current_display_id_ = entry->display.id;
        output_origin_.x = entry->display.x;
        output_origin_.y = entry->display.y;
        return true;
    }

    ScreenCaptureOptions options_;
    Status setup_status_;
    POINT output_origin_{};
    std::vector<DisplayEntry> displays_;
    std::string current_display_id_;
    ComPtr<ID3D11Device> device_;
    ComPtr<ID3D11DeviceContext> context_;
    ComPtr<IDXGIDevice> dxgi_device_;
    ComPtr<IDXGIAdapter> adapter_;
    ComPtr<IDXGIOutput1> output_;
    ComPtr<IDXGIOutputDuplication> duplication_;
};

} // namespace

std::unique_ptr<ScreenCapturerStorage> create_screen_capturer_storage(
    const ScreenCaptureOptions& options) {
    return std::make_unique<DxgiScreenCapturerStorage>(options);
}

ScreenBackendInfo query_screen_backend_info() {
    ScreenBackendInfo info;
    info.available = true;
    info.description =
        "Windows DXGI Desktop Duplication backend for display and visible-window BGRA capture";
    return info;
}

} // namespace nexus::screen::detail
