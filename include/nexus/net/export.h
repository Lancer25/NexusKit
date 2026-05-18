#pragma once

/// Platform import/export annotation for `nexus_net` compiled symbols.
///
/// Expands to `__declspec(dllexport)` when building `nexus_net` as a Windows
/// DLL, `__declspec(dllimport)` when consuming it, and nothing on non-Windows
/// platforms or static builds.
#if defined(_WIN32) && defined(NEXUS_NET_SHARED)
#if defined(NEXUS_NET_BUILDING_LIBRARY)
#define NEXUS_NET_API __declspec(dllexport)
#else
#define NEXUS_NET_API __declspec(dllimport)
#endif
#else
#define NEXUS_NET_API
#endif
