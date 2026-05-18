#pragma once

/// Platform import/export annotation for `nexus_hid` compiled symbols.
///
/// Expands to `__declspec(dllexport)` when building `nexus_hid` as a Windows
/// DLL, `__declspec(dllimport)` when consuming it, and nothing on non-Windows
/// platforms or static builds.
#if defined(_WIN32) && defined(NEXUS_HID_SHARED)
#if defined(NEXUS_HID_BUILDING_LIBRARY)
#define NEXUS_HID_API __declspec(dllexport)
#else
#define NEXUS_HID_API __declspec(dllimport)
#endif
#else
#define NEXUS_HID_API
#endif
