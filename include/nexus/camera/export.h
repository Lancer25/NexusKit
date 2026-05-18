#pragma once

#if defined(_WIN32) && defined(NEXUS_CAMERA_SHARED)
#if defined(NEXUS_CAMERA_BUILDING_LIBRARY)
#define NEXUS_CAMERA_API __declspec(dllexport)
#else
#define NEXUS_CAMERA_API __declspec(dllimport)
#endif
#else
#define NEXUS_CAMERA_API
#endif
