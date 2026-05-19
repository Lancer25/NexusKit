#include <nexus/common/binary.h>

#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace nexus::common {

void write_le16(std::ostream& output, std::uint16_t value) {
    output.put(static_cast<char>(value & 0xff));
    output.put(static_cast<char>((value >> 8) & 0xff));
}

void write_le32(std::ostream& output, std::uint32_t value) {
    output.put(static_cast<char>(value & 0xff));
    output.put(static_cast<char>((value >> 8) & 0xff));
    output.put(static_cast<char>((value >> 16) & 0xff));
    output.put(static_cast<char>((value >> 24) & 0xff));
}

void append_le16(std::vector<std::uint8_t>& data, std::uint16_t value) {
    data.push_back(static_cast<std::uint8_t>(value & 0xff));
    data.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
}

void append_le32(std::vector<std::uint8_t>& data, std::uint32_t value) {
    data.push_back(static_cast<std::uint8_t>(value & 0xff));
    data.push_back(static_cast<std::uint8_t>((value >> 8) & 0xff));
    data.push_back(static_cast<std::uint8_t>((value >> 16) & 0xff));
    data.push_back(static_cast<std::uint8_t>((value >> 24) & 0xff));
}

std::uint16_t read_le16(const std::uint8_t* data, std::size_t data_size, std::size_t offset) {
    if (offset + 2 > data_size) {
        throw std::out_of_range("read_le16: offset + 2 exceeds data_size");
    }
    return static_cast<std::uint16_t>(data[offset]) |
           (static_cast<std::uint16_t>(data[offset + 1]) << 8);
}

std::uint32_t read_le32(const std::uint8_t* data, std::size_t data_size, std::size_t offset) {
    if (offset + 4 > data_size) {
        throw std::out_of_range("read_le32: offset + 4 exceeds data_size");
    }
    return static_cast<std::uint32_t>(data[offset]) |
           (static_cast<std::uint32_t>(data[offset + 1]) << 8) |
           (static_cast<std::uint32_t>(data[offset + 2]) << 16) |
           (static_cast<std::uint32_t>(data[offset + 3]) << 24);
}

static char hex_nibble(std::uint8_t nibble) {
    return static_cast<char>(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
}

static int hex_value(unsigned char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string hex_encode(const std::uint8_t* data, std::size_t length) {
    if (data == nullptr || length == 0) {
        return {};
    }
    std::string result(length * 2, '\0');
    for (std::size_t i = 0; i < length; ++i) {
        result[i * 2] = hex_nibble((data[i] >> 4) & 0x0f);
        result[i * 2 + 1] = hex_nibble(data[i] & 0x0f);
    }
    return result;
}

std::vector<std::uint8_t> hex_decode(std::string_view hex) {
    std::vector<std::uint8_t> result;
    if (hex.size() % 2 != 0) {
        return result;
    }
    result.reserve(hex.size() / 2);
    for (std::size_t i = 0; i < hex.size(); i += 2) {
        const int hi = hex_value(static_cast<unsigned char>(hex[i]));
        const int lo = hex_value(static_cast<unsigned char>(hex[i + 1]));
        if (hi < 0 || lo < 0) {
            return {};
        }
        result.push_back(static_cast<std::uint8_t>((hi << 4) | lo));
    }
    return result;
}

} // namespace nexus::common
