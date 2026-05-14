#pragma once

#if defined(_WIN32) && defined(NEXUS_MEDIA_SHARED)
#if defined(NEXUS_MEDIA_BUILDING_LIBRARY)
#define NEXUS_MEDIA_API __declspec(dllexport)
#else
#define NEXUS_MEDIA_API __declspec(dllimport)
#endif
#else
#define NEXUS_MEDIA_API
#endif
