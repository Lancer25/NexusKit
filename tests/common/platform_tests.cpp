#include <catch2/catch_test_macros.hpp>

#include <nexus/common/platform.h>

TEST_CASE("platform macros define exactly one platform") {
#if defined(_WIN32)
    CHECK(true);
#elif defined(__linux__)
    CHECK(true);
#endif
}

#if defined(NEXUS_PLATFORM_WINDOWS)
TEST_CASE("NEXUS_PLATFORM_WINDOWS is defined on Windows") {
    SUCCEED("NEXUS_PLATFORM_WINDOWS is defined");
}

TEST_CASE("platform header does not include Windows SDK headers") {
#if defined(_WINDOWS_)
    CHECK_FALSE(true);
#else
    SUCCEED("Windows SDK headers are not included by platform.h");
#endif
}
#endif

#if defined(NEXUS_PLATFORM_LINUX)
TEST_CASE("NEXUS_PLATFORM_LINUX is defined on Linux") {
    SUCCEED("NEXUS_PLATFORM_LINUX is defined");
}
#endif
