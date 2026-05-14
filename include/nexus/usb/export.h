#pragma once

#if defined(_WIN32) && defined(NEXUS_USB_SHARED)
#if defined(NEXUS_USB_BUILDING_LIBRARY)
#define NEXUS_USB_API __declspec(dllexport)
#else
#define NEXUS_USB_API __declspec(dllimport)
#endif
#else
#define NEXUS_USB_API
#endif
