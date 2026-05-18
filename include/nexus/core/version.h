#pragma once

#include <string>

#include <nexus/core/export.h>

/// NexusKit version constants and helpers.
namespace nexus::core {

/// Major version number.
constexpr int version_major = 0;
/// Minor version number.
constexpr int version_minor = 1;
/// Patch version number.
constexpr int version_patch = 0;

/// Returns the full version string, e.g. "0.1.0".
NEXUS_CORE_API std::string version_string();

} // namespace nexus::core
