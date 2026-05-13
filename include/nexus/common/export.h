#pragma once

#if defined(_WIN32) && defined(NEXUS_COMMON_SHARED)
#if defined(NEXUS_COMMON_BUILDING_LIBRARY)
#define NEXUS_COMMON_API __declspec(dllexport)
#else
#define NEXUS_COMMON_API __declspec(dllimport)
#endif
#else
#define NEXUS_COMMON_API
#endif
