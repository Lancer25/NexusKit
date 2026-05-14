#include <nexus/usb/usb.h>

#include <string>
#include <vector>

#include <nexus/hid/hid.h>
#include <nexus/log/logger.h>

namespace nexus::usb {

namespace {

hid::HidEnumerationFilter to_hid_filter(UsbEnumerationFilter filter) {
    return hid::HidEnumerationFilter{filter.vendor_id, filter.product_id};
}

UsbDeviceInfo to_usb_device(const hid::HidDeviceInfo& source) {
    UsbDeviceInfo device;
    device.transport = UsbTransport::hid;
    device.path = source.path;
    device.vendor_id = source.vendor_id;
    device.product_id = source.product_id;
    device.serial_number = source.serial_number;
    device.manufacturer = source.manufacturer;
    device.product = source.product;
    device.release_number = source.release_number;
    device.usage_page = source.usage_page;
    device.usage = source.usage;
    device.interface_number = source.interface_number;
    return device;
}

} // namespace

Result<std::vector<UsbDeviceInfo>> enumerate_devices(UsbEnumerationFilter filter) {
    log::write(
        log::Level::debug,
        "USB enumerate vendor_id=" + std::to_string(filter.vendor_id) +
            " product_id=" + std::to_string(filter.product_id));

    const auto hid_devices = hid::enumerate_devices(to_hid_filter(filter));
    if (!hid_devices.ok()) {
        log::write(log::Level::warn, "USB enumerate failed: " + hid_devices.status().message());
        return hid_devices.status();
    }

    std::vector<UsbDeviceInfo> devices;
    devices.reserve(hid_devices.value().size());
    for (const auto& hid_device : hid_devices.value()) {
        devices.push_back(to_usb_device(hid_device));
    }

    log::write(log::Level::info, "USB enumerate count=" + std::to_string(devices.size()));
    return devices;
}

} // namespace nexus::usb
