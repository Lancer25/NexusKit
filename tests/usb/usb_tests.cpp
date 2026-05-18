#include <catch2/catch_test_macros.hpp>

#include <stdexcept>
#include <cstdint>
#include <vector>

#include <nexus/usb/usb.h>

#include "usb_hotplug_private.h"

TEST_CASE("USB enumeration returns a stable device list") {
    const auto devices = nexus::usb::enumerate_devices();

    REQUIRE(devices.ok());
    for (const auto& device : devices.value()) {
        CHECK(device.vendor_id <= static_cast<std::uint16_t>(0xffff));
        CHECK(device.product_id <= static_cast<std::uint16_t>(0xffff));
        CHECK(device.transport == nexus::usb::UsbTransport::hid);
    }
}

TEST_CASE("USB enumeration accepts vendor and product filters") {
    nexus::usb::UsbEnumerationFilter filter;
    filter.vendor_id = 0;
    filter.product_id = 0;

    const auto devices = nexus::usb::enumerate_devices(filter);

    REQUIRE(devices.ok());
}

TEST_CASE("USB device rejects empty HID paths") {
    nexus::usb::UsbDeviceInfo info;
    info.transport = nexus::usb::UsbTransport::hid;
    info.path = "";

    const auto device = nexus::usb::UsbDevice::open(info);

    REQUIRE_FALSE(device.ok());
    CHECK(device.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("USB device reports closed operations") {
    nexus::usb::UsbDevice device;

    CHECK_FALSE(device.is_open());
    CHECK(device.close().ok());

    const auto write_status = device.write({0x00, 0x01});
    CHECK_FALSE(write_status.ok());
    CHECK(write_status.code() == nexus::StatusCode::kFailedPrecondition);

    const auto read_result = device.read(8);
    CHECK_FALSE(read_result.ok());
    CHECK(read_result.status().code() == nexus::StatusCode::kFailedPrecondition);

    const auto feature_status = device.send_feature_report({0x00});
    CHECK_FALSE(feature_status.ok());
    CHECK(feature_status.code() == nexus::StatusCode::kFailedPrecondition);

    const auto feature_result = device.get_feature_report(0, 8);
    CHECK_FALSE(feature_result.ok());
    CHECK(feature_result.status().code() == nexus::StatusCode::kFailedPrecondition);
}

TEST_CASE("UsbHotplugMonitor default state") {
    nexus::usb::UsbHotplugMonitor monitor;
    CHECK_FALSE(monitor.is_running());
    CHECK(monitor.stop().ok());
}

TEST_CASE("UsbHotplugMonitor start and stop") {
    auto result = nexus::usb::UsbHotplugMonitor::start(
        [](const nexus::usb::UsbHotplugEvent&) {});
    REQUIRE(result.ok());

    auto monitor = std::move(result).value();
    CHECK(monitor.is_running());
    CHECK(monitor.stop().ok());
    CHECK_FALSE(monitor.is_running());
}

TEST_CASE("UsbHotplugMonitor rejects empty callback") {
    const auto result = nexus::usb::UsbHotplugMonitor::start({});

    REQUIRE_FALSE(result.ok());
    CHECK(result.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("UsbHotplugMonitor contains callback exceptions") {
    nexus::usb::detail::UsbHotplugStorage storage(
        [](const nexus::usb::UsbHotplugEvent&) {
            throw std::runtime_error("callback failed");
        });

    nexus::usb::UsbDeviceInfo device;
    device.vendor_id = 0x1234;
    device.product_id = 0x5678;

    REQUIRE_NOTHROW(storage.fire_event(device, true));
    CHECK_FALSE(storage.is_running());
}

TEST_CASE("UsbHotplugMonitor double stop is idempotent") {
    auto result = nexus::usb::UsbHotplugMonitor::start(
        [](const nexus::usb::UsbHotplugEvent&) {});
    REQUIRE(result.ok());

    auto monitor = std::move(result).value();
    CHECK(monitor.stop().ok());
    CHECK(monitor.stop().ok());
}

TEST_CASE("UsbHotplugMonitor move transfers ownership") {
    auto result = nexus::usb::UsbHotplugMonitor::start(
        [](const nexus::usb::UsbHotplugEvent&) {});
    REQUIRE(result.ok());

    auto m1 = std::move(result).value();
    CHECK(m1.is_running());

    auto m2 = std::move(m1);
    CHECK_FALSE(m1.is_running());
    CHECK(m2.is_running());

    CHECK(m2.stop().ok());
}

TEST_CASE("USB device validates report sizes") {
    nexus::usb::UsbDevice device;

    const auto empty_write = device.write({});
    CHECK_FALSE(empty_write.ok());
    CHECK(empty_write.code() == nexus::StatusCode::kInvalidArgument);

    const auto empty_feature = device.send_feature_report({});
    CHECK_FALSE(empty_feature.ok());
    CHECK(empty_feature.code() == nexus::StatusCode::kInvalidArgument);

    const auto empty_read = device.read(0);
    CHECK_FALSE(empty_read.ok());
    CHECK(empty_read.status().code() == nexus::StatusCode::kInvalidArgument);

    const auto empty_feature_read = device.get_feature_report(0, 0);
    CHECK_FALSE(empty_feature_read.ok());
    CHECK(empty_feature_read.status().code() == nexus::StatusCode::kInvalidArgument);
}

TEST_CASE("UsbHotplugStorage lifecycle start stop restart") {
    nexus::usb::detail::UsbHotplugStorage storage(
        [](const nexus::usb::UsbHotplugEvent&) {});

    // First cycle
    CHECK(storage.start());
    CHECK(storage.is_running());

    storage.stop();
    CHECK_FALSE(storage.is_running());

    // Second cycle — restart after full stop
    CHECK(storage.start());
    CHECK(storage.is_running());

    storage.stop();
    CHECK_FALSE(storage.is_running());
}

TEST_CASE("UsbHotplugStorage double start returns false") {
    nexus::usb::detail::UsbHotplugStorage storage(
        [](const nexus::usb::UsbHotplugEvent&) {});

    CHECK(storage.start());
    CHECK(storage.is_running());
    CHECK_FALSE(storage.start());
    CHECK(storage.is_running());

    storage.stop();
}

TEST_CASE("UsbHotplugStorage stop without start is safe") {
    nexus::usb::detail::UsbHotplugStorage storage(
        [](const nexus::usb::UsbHotplugEvent&) {});

    CHECK_FALSE(storage.is_running());
    storage.stop();
    CHECK_FALSE(storage.is_running());

    // Start then rely on destructor to stop
    CHECK(storage.start());
}
