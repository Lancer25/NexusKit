#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/usb/export.h>

namespace nexus::usb {

enum class UsbTransport {
    hid
};

struct UsbEnumerationFilter {
    std::uint16_t vendor_id = 0;
    std::uint16_t product_id = 0;
};

struct UsbDeviceInfo {
    UsbTransport transport = UsbTransport::hid;
    std::string path;
    std::uint16_t vendor_id = 0;
    std::uint16_t product_id = 0;
    std::string serial_number;
    std::string manufacturer;
    std::string product;
    std::uint16_t release_number = 0;
    std::uint16_t usage_page = 0;
    std::uint16_t usage = 0;
    int interface_number = -1;
};

NEXUS_USB_API Result<std::vector<UsbDeviceInfo>> enumerate_devices(
    UsbEnumerationFilter filter = {});

} // namespace nexus::usb
