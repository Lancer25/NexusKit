#include <nexus/usb/usb.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <nexus/hid/hid.h>
#include <nexus/log/logger.h>

#include "usb_hotplug_private.h"

namespace nexus::usb {

namespace detail {

class UsbDeviceStorage {
public:
    explicit UsbDeviceStorage(hid::HidDevice device) : device_(std::move(device)) {}

    hid::HidDevice& device() {
        return device_;
    }

    const hid::HidDevice& device() const {
        return device_;
    }

private:
    hid::HidDevice device_;
};

} // namespace detail

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

UsbDevice::UsbDevice() = default;

UsbDevice::UsbDevice(UsbDevice&& other) noexcept = default;

UsbDevice& UsbDevice::operator=(UsbDevice&& other) noexcept = default;

UsbDevice::~UsbDevice() = default;

Result<UsbDevice> UsbDevice::open(const UsbDeviceInfo& info) {
    switch (info.transport) {
    case UsbTransport::hid:
        break;
    }

    auto hid_device = hid::HidDevice::open_path(info.path);
    if (!hid_device.ok()) {
        log::write(log::Level::warn, "USB open failed: " + hid_device.status().message());
        return hid_device.status();
    }

    log::write(log::Level::info, "USB opened path=" + info.path);
    return UsbDevice(std::make_unique<detail::UsbDeviceStorage>(std::move(hid_device).value()));
}

bool UsbDevice::is_open() const {
    return storage_ && storage_->device().is_open();
}

Status UsbDevice::write(const std::vector<std::uint8_t>& report) {
    if (report.empty()) {
        return Status::invalid_argument("USB write report must not be empty");
    }
    if (!storage_) {
        return Status(StatusCode::kFailedPrecondition, "USB device is not open");
    }

    return storage_->device().write(report);
}

Result<std::vector<std::uint8_t>> UsbDevice::read(std::size_t max_bytes, int timeout_ms) {
    if (max_bytes == 0) {
        return Status::invalid_argument("USB read max_bytes must be greater than zero");
    }
    if (!storage_) {
        return Status(StatusCode::kFailedPrecondition, "USB device is not open");
    }

    return storage_->device().read(max_bytes, timeout_ms);
}

Status UsbDevice::send_feature_report(const std::vector<std::uint8_t>& report) {
    if (report.empty()) {
        return Status::invalid_argument("USB feature report must not be empty");
    }
    if (!storage_) {
        return Status(StatusCode::kFailedPrecondition, "USB device is not open");
    }

    return storage_->device().send_feature_report(report);
}

Result<std::vector<std::uint8_t>> UsbDevice::get_feature_report(
    std::uint8_t report_id,
    std::size_t max_bytes) {
    if (max_bytes == 0) {
        return Status::invalid_argument("USB get_feature_report max_bytes must be greater than zero");
    }
    if (!storage_) {
        return Status(StatusCode::kFailedPrecondition, "USB device is not open");
    }

    return storage_->device().get_feature_report(report_id, max_bytes);
}

Status UsbDevice::close() {
    if (!storage_) {
        return Status::ok_status();
    }

    const auto status = storage_->device().close();
    storage_.reset();
    log::write(log::Level::debug, "USB close");
    return status;
}

UsbDevice::UsbDevice(std::unique_ptr<detail::UsbDeviceStorage> storage)
    : storage_(std::move(storage)) {}

UsbHotplugMonitor::UsbHotplugMonitor() = default;
UsbHotplugMonitor::UsbHotplugMonitor(UsbHotplugMonitor&& other) noexcept = default;
UsbHotplugMonitor& UsbHotplugMonitor::operator=(UsbHotplugMonitor&& other) noexcept = default;
UsbHotplugMonitor::~UsbHotplugMonitor() = default;

Result<UsbHotplugMonitor> UsbHotplugMonitor::start(UsbHotplugCallback callback) {
    if (!callback) {
        return Status::invalid_argument("USB hotplug callback cannot be empty");
    }

    auto storage = std::make_unique<detail::UsbHotplugStorage>(std::move(callback));
    if (!storage->start()) {
        return Status::internal("USB hotplug monitor failed to start");
    }

    UsbHotplugMonitor monitor;
    monitor.storage_ = std::move(storage);
    return monitor;
}

bool UsbHotplugMonitor::is_running() const {
    return storage_ && storage_->is_running();
}

Status UsbHotplugMonitor::stop() {
    if (storage_) {
        storage_->stop();
        storage_.reset();
    }
    return Status::ok_status();
}

} // namespace nexus::usb
