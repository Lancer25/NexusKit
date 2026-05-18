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

/// HID device enumeration and I/O backed by hidapi.
///
/// Backend headers are private; users do not need to link against hidapi.
namespace nexus::hid {

namespace detail {
class HidDeviceStorage;
}

/// Filter criteria for `enumerate_devices`.
///
/// Set `vendor_id` or `product_id` to 0 to match all values.
struct HidEnumerationFilter {
    /// USB vendor ID to match, or 0 for any.
    std::uint16_t vendor_id = 0;
    /// USB product ID to match, or 0 for any.
    std::uint16_t product_id = 0;
};

/// Descriptor for an enumerated HID device.
struct HidDeviceInfo {
    /// Opaque backend path.  Use this to open the device.
    std::string path;
    /// USB vendor ID.  0 when unavailable.
    std::uint16_t vendor_id = 0;
    /// USB product ID.  0 when unavailable.
    std::uint16_t product_id = 0;
    /// Device serial number string.  Empty when unavailable.
    std::string serial_number;
    /// Manufacturer string.  Empty when unavailable.
    std::string manufacturer;
    /// Product description string.  Empty when unavailable.
    std::string product;
    /// Device release number in BCD format.  0 when unavailable.
    std::uint16_t release_number = 0;
    /// HID usage page.  0 when unavailable.
    std::uint16_t usage_page = 0;
    /// HID usage.  0 when unavailable.
    std::uint16_t usage = 0;
    /// Interface number within the composite device.  -1 when unavailable.
    int interface_number = -1;
};

/// Enumerates HID devices matching the optional filter.
///
/// @return Device list on success.
/// @retval kFailedPrecondition when the hidapi backend is unavailable.
NEXUS_HID_API Result<std::vector<HidDeviceInfo>> enumerate_devices(
    HidEnumerationFilter filter = {});

/// Move-only RAII HID device.
///
/// Created via `open_path(path)` using a path from `enumerate_devices`.
/// Copy is deleted; move transfers ownership.  A moved-from device is closed
/// and `is_open()` returns false.
///
/// I/O methods may block.  Intended for single-threaded use.
class NEXUS_HID_API HidDevice {
public:
    /// Constructs a closed device.
    HidDevice();
    HidDevice(const HidDevice&) = delete;
    HidDevice& operator=(const HidDevice&) = delete;
    HidDevice(HidDevice&& other) noexcept;
    HidDevice& operator=(HidDevice&& other) noexcept;
    /// Closes the device if open.
    ~HidDevice();

    /// Opens a HID device by backend path.
    ///
    /// @param path A device path from `enumerate_devices`.
    /// @return An open device on success.
    /// @retval kInvalidArgument when `path` is empty.
    /// @retval kNotFound when the device is not present.
    /// @retval kFailedPrecondition when the hidapi backend is unavailable.
    static Result<HidDevice> open_path(std::string path);

    /// True when the device is open.
    bool is_open() const;

    /// Writes an output report to the device.
    ///
    /// The first byte should be the report id (0 for devices without ids).
    /// May block.
    ///
    /// @retval kFailedPrecondition when the device is closed.
    /// @retval kUnavailable on I/O error.
    Status write(const std::vector<std::uint8_t>& report);

    /// Reads an input report from the device.
    ///
    /// May block up to `timeout_ms`.  Set `timeout_ms` to -1 for no timeout.
    ///
    /// @param max_bytes Maximum report size to read.
    /// @param timeout_ms Read timeout in milliseconds, or -1 to block indefinitely.
    /// @retval kFailedPrecondition when the device is closed.
    /// @retval kUnavailable on timeout or I/O error.
    Result<std::vector<std::uint8_t>> read(
        std::size_t max_bytes,
        int timeout_ms = -1);

    /// Sends a feature report to the device.
    ///
    /// The first byte should be the report id.
    ///
    /// @retval kFailedPrecondition when the device is closed.
    /// @retval kUnavailable on I/O error.
    Status send_feature_report(const std::vector<std::uint8_t>& report);

    /// Gets a feature report from the device.
    ///
    /// @param report_id Report id to request.
    /// @param max_bytes Maximum report size to read.
    /// @retval kFailedPrecondition when the device is closed.
    /// @retval kUnavailable on I/O error.
    Result<std::vector<std::uint8_t>> get_feature_report(
        std::uint8_t report_id,
        std::size_t max_bytes);

    /// Closes the device.  Idempotent.
    Status close();

private:
    explicit HidDevice(std::unique_ptr<detail::HidDeviceStorage> storage);

    std::unique_ptr<detail::HidDeviceStorage> storage_;
};

} // namespace nexus::hid
