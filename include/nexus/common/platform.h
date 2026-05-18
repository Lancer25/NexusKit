#pragma once

/// NexusKit platform detection macros.
///
/// These macros detect the target platform at compile time without pulling in
/// native platform SDK headers.  Use them in public headers to gate
/// platform-specific declarations.
///
/// `NEXUS_PLATFORM_WINDOWS` — defined to 1 when building for Windows.
/// `NEXUS_PLATFORM_LINUX`   — defined to 1 when building for Linux.
#if defined(_WIN32)
#define NEXUS_PLATFORM_WINDOWS 1
#elif defined(__linux__)
#define NEXUS_PLATFORM_LINUX 1
#endif
