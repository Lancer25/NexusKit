#pragma once

#include <nexus/common/export.h>

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

/// Little-endian binary I/O and hex encoding helpers.
namespace nexus::common {

/// Writes `value` as a little-endian 16-bit integer to `output`.
NEXUS_COMMON_API void write_le16(std::ostream& output, std::uint16_t value);
/// Writes `value` as a little-endian 32-bit integer to `output`.
NEXUS_COMMON_API void write_le32(std::ostream& output, std::uint32_t value);

/// Appends `value` as little-endian 16-bit bytes to `data`.
NEXUS_COMMON_API void append_le16(std::vector<std::uint8_t>& data, std::uint16_t value);
/// Appends `value` as little-endian 32-bit bytes to `data`.
NEXUS_COMMON_API void append_le32(std::vector<std::uint8_t>& data, std::uint32_t value);

/// Reads a little-endian 16-bit integer from `data` at byte `offset`.
NEXUS_COMMON_API std::uint16_t read_le16(const std::uint8_t* data, std::size_t offset);
/// Reads a little-endian 32-bit integer from `data` at byte `offset`.
NEXUS_COMMON_API std::uint32_t read_le32(const std::uint8_t* data, std::size_t offset);

/// Encodes `length` bytes from `data` as a lowercase hex string.
NEXUS_COMMON_API std::string hex_encode(const std::uint8_t* data, std::size_t length);
/// Decodes a hex string to bytes.
NEXUS_COMMON_API std::vector<std::uint8_t> hex_decode(std::string_view hex);

} // namespace nexus::common
