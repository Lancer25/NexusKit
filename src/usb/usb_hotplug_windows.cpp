#include "usb_hotplug_private.h"

#include <cstdlib>
#include <exception>
#include <string>

#include <nexus/common/string.h>
#include <nexus/log/logger.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dbt.h>

#ifndef _WIN32
#error "This file is Windows-only"
#endif

// {A5DCBF10-6530-11D2-901F-00C04FB951ED}
static const GUID kNexusGuidUsbDeviceInterface = {
    0xA5DCBF10, 0x6530, 0x11D2,
    {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED}};

namespace {

const wchar_t kWindowClassName[] = L"NexusUsbHotplugClass";

nexus::usb::detail::UsbHotplugStorage* g_active_storage = nullptr;

bool parse_vid_pid(const std::string& device_path,
                   std::uint16_t& vid,
                   std::uint16_t& pid) {
    auto upper = device_path;
    for (auto& c : upper) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    const auto vid_pos = upper.find("VID_");
    const auto pid_pos = upper.find("PID_");

    if (vid_pos == std::string::npos || pid_pos == std::string::npos) {
        return false;
    }

    if (vid_pos + 8 > upper.size() || pid_pos + 8 > upper.size()) {
        return false;
    }

    const auto vid_str = upper.substr(vid_pos + 4, 4);
    const auto pid_str = upper.substr(pid_pos + 4, 4);

    char* end = nullptr;
    const auto vid_val = std::strtoul(vid_str.c_str(), &end, 16);
    if (end != vid_str.c_str() + 4) {
        return false;
    }

    const auto pid_val = std::strtoul(pid_str.c_str(), &end, 16);
    if (end != pid_str.c_str() + 4) {
        return false;
    }

    vid = static_cast<std::uint16_t>(vid_val);
    pid = static_cast<std::uint16_t>(pid_val);
    return true;
}

LRESULT CALLBACK hotplug_window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam) {
    if (message == WM_DEVICECHANGE) {
        if (lparam == 0 || g_active_storage == nullptr) {
            return DefWindowProcW(hwnd, message, wparam, lparam);
        }

        auto* header = reinterpret_cast<DEV_BROADCAST_HDR*>(lparam);
        if (header->dbch_devicetype != DBT_DEVTYP_DEVICEINTERFACE) {
            return DefWindowProcW(hwnd, message, wparam, lparam);
        }

        auto* dev_iface = reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE_W*>(lparam);
        const auto device_path = nexus::common::wide_to_utf8(dev_iface->dbcc_name);

        std::uint16_t vid = 0;
        std::uint16_t pid = 0;
        parse_vid_pid(device_path, vid, pid);

        const bool arrived = (wparam == DBT_DEVICEARRIVAL);

        nexus::log::write(
            nexus::log::Level::info,
            "USB hotplug event arrived=" + std::to_string(arrived) +
                " vid=0x" + std::to_string(vid) +
                " pid=0x" + std::to_string(pid));

        nexus::usb::UsbDeviceInfo device;
        device.transport = nexus::usb::UsbTransport::hid;
        device.vendor_id = vid;
        device.product_id = pid;

        g_active_storage->fire_event(std::move(device), arrived);
        return TRUE;
    }

    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wparam, lparam);
}

} // namespace

namespace nexus::usb {
namespace detail {

UsbHotplugStorage::UsbHotplugStorage(UsbHotplugCallback callback)
    : callback_(std::move(callback)) {}

UsbHotplugStorage::~UsbHotplugStorage() {
    stop();
}

bool UsbHotplugStorage::start() {
    return thread_.start([this] { run_platform_loop(); });
}

void UsbHotplugStorage::stop() {
    if (!thread_.is_running()) {
        return;
    }

    // Post WM_QUIT for fast wakeup from PeekMessage loop.
    auto* hwnd = static_cast<HWND>(handle1_);
    if (hwnd != nullptr) {
        PostMessageW(hwnd, WM_QUIT, 0, 0);
    }

    thread_.stop();
    g_active_storage = nullptr;
}

bool UsbHotplugStorage::is_running() const {
    return thread_.is_running();
}

void UsbHotplugStorage::fire_event(UsbDeviceInfo device, bool arrived) {
    if (callback_) {
        UsbHotplugEvent event;
        event.device = std::move(device);
        event.arrived = arrived;
        try {
            callback_(event);
        } catch (const std::exception& ex) {
            nexus::log::write(
                nexus::log::Level::warn,
                std::string("USB hotplug callback threw exception: ") + ex.what());
        } catch (...) {
            nexus::log::write(
                nexus::log::Level::warn,
                "USB hotplug callback threw unknown exception");
        }
    }
}

void UsbHotplugStorage::run_platform_loop() {
    g_active_storage = this;

    // Register window class.
    WNDCLASSW wc = {};
    wc.lpfnWndProc = hotplug_window_proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kWindowClassName;
    RegisterClassW(&wc);

    // Create message-only window.
    auto* hwnd = CreateWindowExW(
        0, kWindowClassName, L"", 0, 0, 0, 0, 0,
        HWND_MESSAGE, nullptr, GetModuleHandleW(nullptr), nullptr);

    if (hwnd == nullptr) {
        nexus::log::write(nexus::log::Level::error, "USB hotplug: CreateWindowEx failed");
        return;
    }

    handle1_ = hwnd;

    // Register device notification.
    DEV_BROADCAST_DEVICEINTERFACE_W filter = {};
    filter.dbcc_size = sizeof(filter);
    filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
    filter.dbcc_classguid = kNexusGuidUsbDeviceInterface;

    auto* notify = RegisterDeviceNotificationW(
        hwnd, &filter, DEVICE_NOTIFY_WINDOW_HANDLE);

    if (notify == nullptr) {
        nexus::log::write(nexus::log::Level::error, "USB hotplug: RegisterDeviceNotification failed");
        DestroyWindow(hwnd);
        handle1_ = nullptr;
        return;
    }

    handle2_ = notify;

    nexus::log::write(nexus::log::Level::info, "USB hotplug monitor started");

    // Message loop using PeekMessage + Sleep so the thread is
    // responsive to stop requests even before the window is created.
    while (!thread_.stop_requested()) {
        MSG msg;
        if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        } else {
            Sleep(50);
        }
    }

    // Cleanup.
    if (handle2_ != nullptr) {
        UnregisterDeviceNotification(static_cast<HDEVNOTIFY>(handle2_));
        handle2_ = nullptr;
    }

    if (handle1_ != nullptr) {
        DestroyWindow(static_cast<HWND>(handle1_));
        handle1_ = nullptr;
    }

    UnregisterClassW(kWindowClassName, GetModuleHandleW(nullptr));

    nexus::log::write(nexus::log::Level::info, "USB hotplug monitor stopped");
}

} // namespace detail
} // namespace nexus::usb
