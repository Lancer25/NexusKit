#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <sstream>
#include <vector>

#include <nexus/common/binary.h>

TEST_CASE("write_le16 outputs little-endian bytes") {
    std::ostringstream output;
    nexus::common::write_le16(output, 0x1234u);
    const auto str = output.str();
    REQUIRE(str.size() == 2);
    CHECK(static_cast<std::uint8_t>(str[0]) == 0x34);
    CHECK(static_cast<std::uint8_t>(str[1]) == 0x12);
}

TEST_CASE("write_le32 outputs little-endian bytes") {
    std::ostringstream output;
    nexus::common::write_le32(output, 0x12345678ul);
    const auto str = output.str();
    REQUIRE(str.size() == 4);
    CHECK(static_cast<std::uint8_t>(str[0]) == 0x78);
    CHECK(static_cast<std::uint8_t>(str[1]) == 0x56);
    CHECK(static_cast<std::uint8_t>(str[2]) == 0x34);
    CHECK(static_cast<std::uint8_t>(str[3]) == 0x12);
}

TEST_CASE("append_le16 appends little-endian bytes") {
    std::vector<std::uint8_t> data;
    nexus::common::append_le16(data, 0xabcd);
    REQUIRE(data.size() == 2);
    CHECK(data[0] == 0xcd);
    CHECK(data[1] == 0xab);
}

TEST_CASE("append_le32 appends little-endian bytes") {
    std::vector<std::uint8_t> data;
    nexus::common::append_le32(data, 0xdeadbeeful);
    REQUIRE(data.size() == 4);
    CHECK(data[0] == 0xef);
    CHECK(data[1] == 0xbe);
    CHECK(data[2] == 0xad);
    CHECK(data[3] == 0xde);
}

TEST_CASE("read_le16 reads little-endian bytes") {
    const std::uint8_t data[] = {0x78, 0x56};
    CHECK(nexus::common::read_le16(data, sizeof(data), 0) == 0x5678);
}

TEST_CASE("read_le32 reads little-endian bytes") {
    const std::uint8_t data[] = {0x78, 0x56, 0x34, 0x12};
    CHECK(nexus::common::read_le32(data, sizeof(data), 0) == 0x12345678);
}

TEST_CASE("hex_encode produces lowercase hex string") {
    const std::uint8_t data[] = {0x00, 0xff, 0xab, 0x0f};
    CHECK(nexus::common::hex_encode(data, 4) == "00ffab0f");
}

TEST_CASE("hex_encode handles empty input") {
    CHECK(nexus::common::hex_encode(nullptr, 0) == "");
    const std::uint8_t data[] = {0x42};
    CHECK(nexus::common::hex_encode(data, 0) == "");
}

TEST_CASE("hex_decode produces bytes from hex string") {
    const auto bytes = nexus::common::hex_decode("00ffab0f");
    REQUIRE(bytes.size() == 4);
    CHECK(bytes[0] == 0x00);
    CHECK(bytes[1] == 0xff);
    CHECK(bytes[2] == 0xab);
    CHECK(bytes[3] == 0x0f);
}

TEST_CASE("hex_decode handles uppercase") {
    const auto bytes = nexus::common::hex_decode("A1B2C3");
    REQUIRE(bytes.size() == 3);
    CHECK(bytes[0] == 0xa1);
    CHECK(bytes[1] == 0xb2);
    CHECK(bytes[2] == 0xc3);
}

TEST_CASE("hex_decode rejects odd-length input") {
    const auto bytes = nexus::common::hex_decode("abc");
    CHECK(bytes.empty());
}

TEST_CASE("hex_decode rejects invalid characters") {
    const auto bytes = nexus::common::hex_decode("xxyy");
    CHECK(bytes.empty());
}

TEST_CASE("hex_encode and hex_decode round-trip") {
    const std::uint8_t original[] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};
    const auto hex = nexus::common::hex_encode(original, 8);
    const auto decoded = nexus::common::hex_decode(hex);
    REQUIRE(decoded.size() == 8);
    for (size_t i = 0; i < 8; ++i) {
        CHECK(decoded[i] == original[i]);
    }
}
