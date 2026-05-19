#pragma once

#include <nexus/common/export.h>

#include <cstdint>

/// Monotonic and wall-clock timestamp helpers.
namespace nexus::common {

/// Monotonic timestamp in milliseconds, suitable for measuring intervals.
///
/// Uses `std::chrono::steady_clock`.  Not related to wall-clock time.
NEXUS_COMMON_API std::int64_t steady_timestamp_ms();

/// Wall-clock epoch timestamp in milliseconds (Unix epoch).
///
/// Uses `std::chrono::system_clock`.  Subject to system clock adjustments.
NEXUS_COMMON_API std::int64_t system_timestamp_ms();

} // namespace nexus::common
