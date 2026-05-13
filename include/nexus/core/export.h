#pragma once

#if defined(_WIN32) && defined(NEXUS_CORE_SHARED)
#if defined(NEXUS_CORE_BUILDING_LIBRARY)
#define NEXUS_CORE_API __declspec(dllexport)
#else
#define NEXUS_CORE_API __declspec(dllimport)
#endif
#else
#define NEXUS_CORE_API
#endif
