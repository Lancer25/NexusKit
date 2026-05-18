#pragma once

#include <functional>
#include <memory>
#include <string>

#include <nexus/common/thread.h>
#include <nexus/usb/usb.h>

namespace nexus::usb {
namespace detail {

class UsbHotplugStorage {
public:
    explicit UsbHotplugStorage(UsbHotplugCallback callback);
    ~UsbHotplugStorage();

    UsbHotplugStorage(const UsbHotplugStorage&) = delete;
    UsbHotplugStorage& operator=(const UsbHotplugStorage&) = delete;

    bool start();
    void stop();
    bool is_running() const;
    void fire_event(UsbDeviceInfo device, bool arrived);

private:
    void run_platform_loop();

    UsbHotplugCallback callback_;
    nexus::common::Thread thread_;

    // Opaque platform handles.
    // Windows: handle1_ = HWND (message-only window), handle2_ = HDEVNOTIFY.
    // Linux:   handle1_ = struct udev*, handle2_ = struct udev_monitor*.
    void* handle1_ = nullptr;
    void* handle2_ = nullptr;
};

} // namespace detail
} // namespace nexus::usb
