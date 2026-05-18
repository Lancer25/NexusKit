#include <nexus/hid/hid.h>

#include <climits>
#include <cstdlib>
#include <cwchar>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <hidapi.h>

#include <nexus/common/string.h>

#include <nexus/core/status.h>
#include <nexus/log/logger.h>

namespace nexus::hid {

namespace detail {

class HidDeviceStorage {
public:
    explicit HidDeviceStorage(hid_device* device) : device_(device) {}

    HidDeviceStorage(const HidDeviceStorage&) = delete;
    HidDeviceStorage& operator=(const HidDeviceStorage&) = delete;

    ~HidDeviceStorage() {
        close();
    }

    hid_device* device() const {
        return device_;
    }

    bool is_open() const {
        return device_ != nullptr;
    }

    void close() {
        if (device_ != nullptr) {
            hid_close(device_);
            device_ = nullptr;
        }
    }

private:
    hid_device* device_ = nullptr;
};

} // namespace detail

namespace {

HidDeviceInfo to_device_info(const hid_device_info& source) {
    HidDeviceInfo device;
    device.path = nexus::common::char_to_string(source.path);
    device.vendor_id = source.vendor_id;
    device.product_id = source.product_id;
    device.serial_number = source.serial_number
        ? nexus::common::wide_to_utf8(source.serial_number) : std::string();
    device.manufacturer = source.manufacturer_string
        ? nexus::common::wide_to_utf8(source.manufacturer_string) : std::string();
    device.product = source.product_string
        ? nexus::common::wide_to_utf8(source.product_string) : std::string();
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

Status closed_status() {
    return Status(StatusCode::kFailedPrecondition, "HID device is not open");
}

Status io_status(std::string message) {
    return Status(StatusCode::kUnavailable, std::move(message));
}

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

HidDevice::HidDevice() = default;

HidDevice::HidDevice(HidDevice&& other) noexcept = default;

HidDevice& HidDevice::operator=(HidDevice&& other) noexcept = default;

HidDevice::~HidDevice() = default;

Result<HidDevice> HidDevice::open_path(std::string path) {
    if (path.empty()) {
        return Status::invalid_argument("HID path cannot be empty");
    }

    if (hid_init() != 0) {
        log::write(log::Level::warn, "HID initialization failed");
        return Status::internal("HID initialization failed");
    }

    log::write(log::Level::debug, "HID open path=" + path);
    auto* handle = hid_open_path(path.c_str());
    if (handle == nullptr) {
        log::write(log::Level::warn, "HID open failed path=" + path);
        return io_status("HID open failed");
    }

    log::write(log::Level::info, "HID opened path=" + path);
    return HidDevice(std::make_unique<detail::HidDeviceStorage>(handle));
}

bool HidDevice::is_open() const {
    return storage_ && storage_->is_open();
}

Status HidDevice::write(const std::vector<std::uint8_t>& report) {
    if (report.empty()) {
        return Status::invalid_argument("HID write report cannot be empty");
    }

    if (!is_open()) {
        return closed_status();
    }

    const auto written = hid_write(storage_->device(), report.data(), report.size());
    if (written < 0) {
        log::write(log::Level::warn, "HID write failed");
        return io_status("HID write failed");
    }

    log::write(log::Level::debug, "HID write bytes=" + std::to_string(written));
    return Status::ok_status();
}

Result<std::vector<std::uint8_t>> HidDevice::read(std::size_t max_bytes, int timeout_ms) {
    if (max_bytes == 0) {
        return Status::invalid_argument("HID read size must be greater than zero");
    }

    if (!is_open()) {
        return closed_status();
    }

    std::vector<std::uint8_t> buffer(max_bytes);
    const auto bytes_read = timeout_ms >= 0
        ? hid_read_timeout(storage_->device(), buffer.data(), buffer.size(), timeout_ms)
        : hid_read(storage_->device(), buffer.data(), buffer.size());
    if (bytes_read < 0) {
        log::write(log::Level::warn, "HID read failed");
        return io_status("HID read failed");
    }

    buffer.resize(static_cast<std::size_t>(bytes_read));
    log::write(log::Level::debug, "HID read bytes=" + std::to_string(bytes_read));
    return buffer;
}

Status HidDevice::send_feature_report(const std::vector<std::uint8_t>& report) {
    if (report.empty()) {
        return Status::invalid_argument("HID feature report cannot be empty");
    }

    if (!is_open()) {
        return closed_status();
    }

    const auto written = hid_send_feature_report(storage_->device(), report.data(), report.size());
    if (written < 0) {
        log::write(log::Level::warn, "HID send feature report failed");
        return io_status("HID send feature report failed");
    }

    log::write(log::Level::debug, "HID send feature report bytes=" + std::to_string(written));
    return Status::ok_status();
}

Result<std::vector<std::uint8_t>> HidDevice::get_feature_report(
    std::uint8_t report_id,
    std::size_t max_bytes) {
    if (max_bytes == 0) {
        return Status::invalid_argument("HID feature report size must be greater than zero");
    }

    if (!is_open()) {
        return closed_status();
    }

    std::vector<std::uint8_t> buffer(max_bytes);
    buffer[0] = report_id;
    const auto bytes_read = hid_get_feature_report(storage_->device(), buffer.data(), buffer.size());
    if (bytes_read < 0) {
        log::write(log::Level::warn, "HID get feature report failed");
        return io_status("HID get feature report failed");
    }

    buffer.resize(static_cast<std::size_t>(bytes_read));
    log::write(log::Level::debug, "HID get feature report bytes=" + std::to_string(bytes_read));
    return buffer;
}

Status HidDevice::close() {
    if (storage_) {
        storage_->close();
        storage_.reset();
        log::write(log::Level::debug, "HID close");
    }

    return Status::ok_status();
}

HidDevice::HidDevice(std::unique_ptr<detail::HidDeviceStorage> storage)
    : storage_(std::move(storage)) {}

} // namespace nexus::hid
