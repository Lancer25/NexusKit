#include <nexus/hid/hid.h>

#include <climits>
#include <cstdlib>
#include <cwchar>
#include <memory>
#include <string>
#include <vector>

#include <hidapi.h>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

#include <nexus/core/status.h>
#include <nexus/log/logger.h>

namespace nexus::hid {

namespace {

std::string wide_to_utf8(const wchar_t* value) {
    if (value == nullptr) {
        return {};
    }

#if defined(_WIN32)
    const auto length = WideCharToMultiByte(CP_UTF8, 0, value, -1, nullptr, 0, nullptr, nullptr);
    if (length <= 1) {
        return {};
    }

    std::string result(static_cast<std::size_t>(length - 1), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value, -1, result.data(), length, nullptr, nullptr);
    return result;
#else
    std::mbstate_t state{};
    const wchar_t* cursor = value;
    const auto length = std::wcsrtombs(nullptr, &cursor, 0, &state);
    if (length == static_cast<std::size_t>(-1)) {
        return {};
    }

    std::string result(length, '\0');
    state = std::mbstate_t{};
    cursor = value;
    std::wcsrtombs(result.data(), &cursor, result.size(), &state);
    return result;
#endif
}

std::string char_to_string(const char* value) {
    return value == nullptr ? std::string() : std::string(value);
}

HidDeviceInfo to_device_info(const hid_device_info& source) {
    HidDeviceInfo device;
    device.path = char_to_string(source.path);
    device.vendor_id = source.vendor_id;
    device.product_id = source.product_id;
    device.serial_number = wide_to_utf8(source.serial_number);
    device.manufacturer = wide_to_utf8(source.manufacturer_string);
    device.product = wide_to_utf8(source.product_string);
    device.release_number = source.release_number;
    device.usage_page = source.usage_page;
    device.usage = source.usage;
    device.interface_number = source.interface_number;
    return device;
}

struct HidEnumerationDeleter {
    void operator()(hid_device_info* devices) const {
        hid_free_enumeration(devices);
    }
};

using HidEnumeration = std::unique_ptr<hid_device_info, HidEnumerationDeleter>;

} // namespace

Result<std::vector<HidDeviceInfo>> enumerate_devices(HidEnumerationFilter filter) {
    log::write(
        log::Level::debug,
        "HID enumerate vendor_id=" + std::to_string(filter.vendor_id) +
            " product_id=" + std::to_string(filter.product_id));

    if (hid_init() != 0) {
        log::write(log::Level::warn, "HID initialization failed");
        return Status::internal("HID initialization failed");
    }

    HidEnumeration enumeration(hid_enumerate(filter.vendor_id, filter.product_id));

    std::vector<HidDeviceInfo> devices;
    for (auto* current = enumeration.get(); current != nullptr; current = current->next) {
        devices.push_back(to_device_info(*current));
    }

    log::write(log::Level::info, "HID enumerate count=" + std::to_string(devices.size()));
    return devices;
}

} // namespace nexus::hid
