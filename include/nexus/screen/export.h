#pragma once

#if defined(_WIN32) && defined(NEXUS_SCREEN_SHARED)
#if defined(NEXUS_SCREEN_BUILDING_LIBRARY)
#define NEXUS_SCREEN_API __declspec(dllexport)
#else
#define NEXUS_SCREEN_API __declspec(dllimport)
#endif
#else
#define NEXUS_SCREEN_API
#endif
