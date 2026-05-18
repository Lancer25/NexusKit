#include "usb_hotplug_private.h"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <string>

#include <nexus/log/logger.h>

#include <libudev.h>
#include <poll.h>
#include <unistd.h>

#ifndef __linux__
#error "This file is Linux-only"
#endif

namespace {

unsigned int hex_string_to_uint(const char* str) {
    if (str == nullptr) {
        return 0;
    }
    char* end = nullptr;
    const auto val = std::strtoul(str, &end, 16);
    if (end == str) {
        return 0;
    }
    return static_cast<unsigned int>(val);
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

    thread_.stop();
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
    auto* udev = udev_new();
    if (udev == nullptr) {
        nexus::log::write(nexus::log::Level::error, "USB hotplug: udev_new failed");
        return;
    }

    handle1_ = udev;

    auto* mon = udev_monitor_new_from_netlink(udev, "udev");
    if (mon == nullptr) {
        nexus::log::write(nexus::log::Level::error, "USB hotplug: udev_monitor_new_from_netlink failed");
        udev_unref(udev);
        handle1_ = nullptr;
        return;
    }

    handle2_ = mon;

    udev_monitor_filter_add_match_subsystem_devtype(mon, "usb", nullptr);
    udev_monitor_enable_receiving(mon);

    const auto fd = udev_monitor_get_fd(mon);
    if (fd < 0) {
        nexus::log::write(nexus::log::Level::error, "USB hotplug: udev_monitor_get_fd failed");
        udev_monitor_unref(mon);
        handle2_ = nullptr;
        udev_unref(udev);
        handle1_ = nullptr;
        return;
    }

    nexus::log::write(nexus::log::Level::info, "USB hotplug monitor started");

    while (!thread_.stop_requested()) {
        pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;

        const auto ret = poll(&pfd, 1, 200);
        if (ret < 0) {
            break;
        }

        if (ret == 0 || !(pfd.revents & POLLIN)) {
            continue;
        }

        auto* dev = udev_monitor_receive_device(mon);
        if (dev == nullptr) {
            continue;
        }

        const char* action = udev_device_get_action(dev);
        if (action == nullptr) {
            udev_device_unref(dev);
            continue;
        }

        const bool arrived = (std::strcmp(action, "add") == 0);
        const bool removed = (std::strcmp(action, "remove") == 0);
        if (!arrived && !removed) {
            udev_device_unref(dev);
            continue;
        }

        const auto* vid_str = udev_device_get_property_value(dev, "ID_VENDOR_ID");
        const auto* pid_str = udev_device_get_property_value(dev, "ID_MODEL_ID");

        const auto vid = static_cast<std::uint16_t>(hex_string_to_uint(vid_str));
        const auto pid = static_cast<std::uint16_t>(hex_string_to_uint(pid_str));

        nexus::log::write(
            nexus::log::Level::info,
            "USB hotplug event arrived=" + std::to_string(arrived) +
                " vid=0x" + std::to_string(vid) +
                " pid=0x" + std::to_string(pid));

        UsbDeviceInfo device;
        device.transport = UsbTransport::hid;
        device.vendor_id = vid;
        device.product_id = pid;

        fire_event(std::move(device), arrived);

        udev_device_unref(dev);
    }

    if (handle2_ != nullptr) {
        udev_monitor_unref(static_cast<udev_monitor*>(handle2_));
        handle2_ = nullptr;
    }

    if (handle1_ != nullptr) {
        udev_unref(static_cast<udev*>(handle1_));
        handle1_ = nullptr;
    }

    nexus::log::write(nexus::log::Level::info, "USB hotplug monitor stopped");
}

} // namespace detail
} // namespace nexus::usb
