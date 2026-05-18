#pragma once

#include <string>

#include <nexus/common/export.h>
#include <nexus/log/logger.h>

/// Diagnostic logging routed through `nexus_log`.
namespace nexus::common {

/// Writes a log message through the `nexus::log` backend.
///
/// This is a lightweight bridge for code that wants to produce diagnostics
/// without directly depending on all of `nexus_log`.  The behavior (sinks,
/// formatting, thread safety) is controlled by the `nexus_log` configuration
/// active at runtime.
///
/// @param level Severity level.
/// @param message The message to log.
NEXUS_COMMON_API void diagnostic_log(nexus::log::Level level,
                                     const std::string& message);

} // namespace nexus::common
