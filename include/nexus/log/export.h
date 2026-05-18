#pragma once

/// Platform import/export annotation for `nexus_log` compiled symbols.
///
/// Expands to `__declspec(dllexport)` when building `nexus_log` as a Windows
/// DLL, `__declspec(dllimport)` when consuming it, and nothing on non-Windows
/// platforms or static builds.
#if defined(_WIN32) && defined(NEXUS_LOG_SHARED)
#if defined(NEXUS_LOG_BUILDING_LIBRARY)
#define NEXUS_LOG_API __declspec(dllexport)
#else
#define NEXUS_LOG_API __declspec(dllimport)
#endif
#else
#define NEXUS_LOG_API
#endif
