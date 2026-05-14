#pragma once

#if defined(_WIN32) && defined(NEXUS_HID_SHARED)
#if defined(NEXUS_HID_BUILDING_LIBRARY)
#define NEXUS_HID_API __declspec(dllexport)
#else
#define NEXUS_HID_API __declspec(dllimport)
#endif
#else
#define NEXUS_HID_API
#endif
