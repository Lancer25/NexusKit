#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <nexus/core/result.h>
#include <nexus/core/status.h>
#include <nexus/usb/export.h>

/// USB device enumeration and I/O.
///
/// Currently supports HID-class USB devices via the OS HID API.
/// Backend headers are private.
namespace nexus::usb {

namespace detail {
class UsbDeviceStorage;
class UsbHotplugStorage;
}

/// USB device transport type.
enum class UsbTransport {
    /// HID-class device accessed via the OS HID layer.
    hid
};

/// Filter criteria for `enumerate_devices`.
///
/// Set `vendor_id` or `product_id` to 0 to match all values.
struct UsbEnumerationFilter {
    /// USB vendor ID to match, or 0 for any.
    std::uint16_t vendor_id = 0;
    /// USB product ID to match, or 0 for any.
    std::uint16_t product_id = 0;
};

/// Descriptor for an enumerated USB HID device.
struct UsbDeviceInfo {
    /// Transport type used to access this device.
    UsbTransport transport = UsbTransport::hid;
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

/// Enumerates USB HID devices matching the optional filter.
///
/// @return Device list on success.
/// @retval kFailedPrecondition when the HID backend is unavailable.
NEXUS_USB_API Result<std::vector<UsbDeviceInfo>> enumerate_devices(
    UsbEnumerationFilter filter = {});

/// Move-only RAII USB HID device.
///
/// Created via `open(info)` using a `UsbDeviceInfo` from `enumerate_devices`.
/// Copy is deleted; move transfers ownership.  A moved-from device is closed
/// and `is_open()` returns false.
///
/// I/O methods may block.  Intended for single-threaded use.
class NEXUS_USB_API UsbDevice {
public:
    /// Constructs a closed device.
    UsbDevice();
    /// USB devices are move-only and cannot be copied.
    UsbDevice(const UsbDevice&) = delete;
    /// USB devices are move-only and cannot be copy-assigned.
    UsbDevice& operator=(const UsbDevice&) = delete;
    /// Moves a device handle.
    UsbDevice(UsbDevice&& other) noexcept;
    /// Moves a device handle.
    UsbDevice& operator=(UsbDevice&& other) noexcept;
    /// Closes the device if open.
    ~UsbDevice();

    /// Opens a USB HID device.
    ///
    /// @param info A `UsbDeviceInfo` from `enumerate_devices`.
    /// @return An open device on success.
    /// @retval kInvalidArgument when `info.path` is empty.
    /// @retval kNotFound when the device is not present.
    /// @retval kFailedPrecondition when the HID backend is unavailable.
    static Result<UsbDevice> open(const UsbDeviceInfo& info);

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
    explicit UsbDevice(std::unique_ptr<detail::UsbDeviceStorage> storage);

    std::unique_ptr<detail::UsbDeviceStorage> storage_;
};

/// USB device hotplug event.
struct UsbHotplugEvent {
    /// Device that was added or removed.
    UsbDeviceInfo device;
    /// True when the device arrived, false when removed.
    bool arrived = false;
};

/// Callback for USB hotplug notifications.
///
/// Called from a background thread; implementations must be thread-safe and
/// should return quickly.  Exceptions thrown by the callback are caught and
/// logged by the monitor.
using UsbHotplugCallback = std::function<void(const UsbHotplugEvent& event)>;

/// Move-only RAII USB hotplug monitor.
///
/// Created via `start(callback)`.  The callback is invoked from a background
/// thread when USB devices are plugged in or removed.
///
/// Copy is deleted; move transfers ownership.  A moved-from monitor is stopped.
class NEXUS_USB_API UsbHotplugMonitor {
public:
    /// Constructs a stopped monitor.
    UsbHotplugMonitor();
    /// Hotplug monitors are move-only and cannot be copied.
    UsbHotplugMonitor(const UsbHotplugMonitor&) = delete;
    /// Hotplug monitors are move-only and cannot be copy-assigned.
    UsbHotplugMonitor& operator=(const UsbHotplugMonitor&) = delete;
    /// Moves a monitor handle.
    UsbHotplugMonitor(UsbHotplugMonitor&& other) noexcept;
    /// Moves a monitor handle.
    UsbHotplugMonitor& operator=(UsbHotplugMonitor&& other) noexcept;
    /// Stops the monitor if running.
    ~UsbHotplugMonitor();

    /// Starts monitoring USB device hotplug events.
    ///
    /// The callback is invoked from a background thread on every device
    /// arrival or removal.  An empty callback is rejected.
    ///
    /// @return A running monitor on success.
    /// @retval kInvalidArgument when `callback` is empty.
    /// @retval kFailedPrecondition when the HID backend is unavailable.
    /// @retval kInternal when the platform monitor cannot be created.
    static Result<UsbHotplugMonitor> start(UsbHotplugCallback callback);

    /// True when the monitor is running.
    bool is_running() const;

    /// Stops the monitor and waits for the background thread to exit.
    /// Idempotent; safe to call on a stopped monitor.
    Status stop();

private:
    std::unique_ptr<detail::UsbHotplugStorage> storage_;
};

} // namespace nexus::usb
