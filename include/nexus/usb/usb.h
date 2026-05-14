#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/usb/export.h>

namespace nexus::usb {

namespace detail {
class UsbDeviceStorage;
}

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

class NEXUS_USB_API UsbDevice {
public:
    UsbDevice();
    UsbDevice(const UsbDevice&) = delete;
    UsbDevice& operator=(const UsbDevice&) = delete;
    UsbDevice(UsbDevice&& other) noexcept;
    UsbDevice& operator=(UsbDevice&& other) noexcept;
    ~UsbDevice();

    static Result<UsbDevice> open(const UsbDeviceInfo& info);

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
    explicit UsbDevice(std::unique_ptr<detail::UsbDeviceStorage> storage);

    std::unique_ptr<detail::UsbDeviceStorage> storage_;
};

} // namespace nexus::usb
