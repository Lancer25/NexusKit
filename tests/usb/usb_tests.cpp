#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

#include <nexus/usb/usb.h>

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
