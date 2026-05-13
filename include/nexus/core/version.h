#pragma once

#include <string>

#include <nexus/core/export.h>

namespace nexus::core {

constexpr int version_major = 0;
constexpr int version_minor = 1;
constexpr int version_patch = 0;

NEXUS_CORE_API std::string version_string();

} // namespace nexus::core
