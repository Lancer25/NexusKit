#pragma once

#if defined(_WIN32) && defined(NEXUS_AUDIO_SHARED)
#if defined(NEXUS_AUDIO_BUILDING_LIBRARY)
#define NEXUS_AUDIO_API __declspec(dllexport)
#else
#define NEXUS_AUDIO_API __declspec(dllimport)
#endif
#else
#define NEXUS_AUDIO_API
#endif
