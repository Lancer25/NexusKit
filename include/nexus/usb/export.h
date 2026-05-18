#pragma once

/// Platform import/export annotation for `nexus_usb` compiled symbols.
///
/// Expands to `__declspec(dllexport)` when building `nexus_usb` as a Windows
/// DLL, `__declspec(dllimport)` when consuming it, and nothing on non-Windows
/// platforms or static builds.
#if defined(_WIN32) && defined(NEXUS_USB_SHARED)
#if defined(NEXUS_USB_BUILDING_LIBRARY)
#define NEXUS_USB_API __declspec(dllexport)
#else
#define NEXUS_USB_API __declspec(dllimport)
#endif
#else
#define NEXUS_USB_API
#endif
