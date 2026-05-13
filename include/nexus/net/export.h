#pragma once

#if defined(_WIN32) && defined(NEXUS_NET_SHARED)
#if defined(NEXUS_NET_BUILDING_LIBRARY)
#define NEXUS_NET_API __declspec(dllexport)
#else
#define NEXUS_NET_API __declspec(dllimport)
#endif
#else
#define NEXUS_NET_API
#endif
