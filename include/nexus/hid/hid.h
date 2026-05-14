#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/hid/export.h>

namespace nexus::hid {

struct HidEnumerationFilter {
    std::uint16_t vendor_id = 0;
    std::uint16_t product_id = 0;
};

struct HidDeviceInfo {
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

NEXUS_HID_API Result<std::vector<HidDeviceInfo>> enumerate_devices(
    HidEnumerationFilter filter = {});

} // namespace nexus::hid
