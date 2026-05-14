#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/hid/export.h>

namespace nexus::hid {

namespace detail {
class HidDeviceStorage;
}

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

class NEXUS_HID_API HidDevice {
public:
    HidDevice();
    HidDevice(const HidDevice&) = delete;
    HidDevice& operator=(const HidDevice&) = delete;
    HidDevice(HidDevice&& other) noexcept;
    HidDevice& operator=(HidDevice&& other) noexcept;
    ~HidDevice();

    static Result<HidDevice> open_path(std::string path);

    bool is_open() const;
    Status write(const std::vector<std::uint8_t>& report);
    Result<std::vector<std::uint8_t>> read(
        std::size_t max_bytes,
        int timeout_ms = -1);
    Status send_feature_report(const std::vector<std::uint8_t>& report);
    Result<std::vector<std::uint8_t>> get_feature_report(
        std::uint8_t report_id,
        std::size_t max_bytes);
    Status close();

private:
    explicit HidDevice(std::unique_ptr<detail::HidDeviceStorage> storage);

    std::unique_ptr<detail::HidDeviceStorage> storage_;
};

} // namespace nexus::hid
