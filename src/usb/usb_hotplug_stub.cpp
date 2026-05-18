#include "usb_hotplug_private.h"

#include <exception>
#include <string>

#include <nexus/log/logger.h>

namespace nexus::usb {
namespace detail {

UsbHotplugStorage::UsbHotplugStorage(UsbHotplugCallback callback)
    : callback_(std::move(callback)) {}

UsbHotplugStorage::~UsbHotplugStorage() {
    stop();
}

bool UsbHotplugStorage::start() {
    nexus::log::write(nexus::log::Level::warn,
                      "USB hotplug is not supported on this platform");
    return false;
}

void UsbHotplugStorage::stop() {
    thread_.stop();
}

bool UsbHotplugStorage::is_running() const {
    return false;
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

} // namespace detail
} // namespace nexus::usb
