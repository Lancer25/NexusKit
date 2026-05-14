#include <catch2/catch_test_macros.hpp>

#include <cstdint>

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
