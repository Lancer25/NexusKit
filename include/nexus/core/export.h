#pragma once

/// Platform import/export annotation for `nexus_core` compiled symbols.
///
/// Expands to `__declspec(dllexport)` when building `nexus_core` as a Windows
/// DLL, `__declspec(dllimport)` when consuming it, and nothing on non-Windows
/// platforms or static builds.
#if defined(_WIN32) && defined(NEXUS_CORE_SHARED)
#if defined(NEXUS_CORE_BUILDING_LIBRARY)
#define NEXUS_CORE_API __declspec(dllexport)
#else
#define NEXUS_CORE_API __declspec(dllimport)
#endif
#else
#define NEXUS_CORE_API
#endif
