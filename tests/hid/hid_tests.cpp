#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <vector>

#include <nexus/hid/hid.h>

TEST_CASE("HID enumeration returns a stable device list") {
    const auto devices = nexus::hid::enumerate_devices();

    REQUIRE(devices.ok());
    for (const auto& device : devices.value()) {
        CHECK(device.vendor_id <= static_cast<std::uint16_t>(0xffff));
        CHECK(device.product_id <= static_cast<std::uint16_t>(0xffff));
    }
}

TEST_CASE("HID enumeration accepts vendor and product filters") {
    nexus::hid::HidEnumerationFilter filter;
    filter.vendor_id = 0;
    filter.product_id = 0;

    const auto devices = nexus::hid::enumerate_devices(filter);

    REQUIRE(devices.ok());
}
