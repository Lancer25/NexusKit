#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "screen_private.h"

#include <d3d11.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <windows.h>

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

std::int64_t steady_timestamp_ms() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

std::int64_t qpc_timestamp_ms(const LARGE_INTEGER timestamp) {
    if (timestamp.QuadPart == 0) {
        return steady_timestamp_ms();
    }

    LARGE_INTEGER frequency{};
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart == 0) {
        return steady_timestamp_ms();
    }

    return static_cast<std::int64_t>((timestamp.QuadPart * 1000) / frequency.QuadPart);
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

    Result<ScreenFrame> capture_primary() override {
        if (!is_available()) {
            return setup_status_.ok()
                       ? failed_precondition("screen capture backend is not available")
                       : setup_status_;
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

        return frame;
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

        ComPtr<IDXGIOutput> dxgi_output;
        hr = adapter_->EnumOutputs(0, dxgi_output.GetAddressOf());
        if (FAILED(hr)) {
            return map_setup_failure(hr, "EnumOutputs");
        }

        DXGI_OUTPUT_DESC output_desc{};
        hr = dxgi_output->GetDesc(&output_desc);
        if (FAILED(hr)) {
            return internal("failed to query primary output description");
        }
        if (!output_desc.AttachedToDesktop) {
            return failed_precondition("primary output is not attached to the desktop");
        }

        hr = dxgi_output.As(&output_);
        if (FAILED(hr)) {
            return map_setup_failure(hr, "IDXGIOutput1 query");
        }

        hr = output_->DuplicateOutput(device_.Get(), duplication_.GetAddressOf());
        if (FAILED(hr)) {
            return map_setup_failure(hr, "DuplicateOutput");
        }

        return Status::ok_status();
    }

    ScreenCaptureOptions options_;
    Status setup_status_;
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
        "Windows DXGI Desktop Duplication backend for primary display BGRA capture";
    return info;
}

} // namespace nexus::screen::detail
