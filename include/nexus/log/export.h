#pragma once

#if defined(_WIN32) && defined(NEXUS_LOG_SHARED)
#if defined(NEXUS_LOG_BUILDING_LIBRARY)
#define NEXUS_LOG_API __declspec(dllexport)
#else
#define NEXUS_LOG_API __declspec(dllimport)
#endif
#else
#define NEXUS_LOG_API
#endif
