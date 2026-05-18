include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS)
    nexus_print_dependency_mode(cpp-httplib "FetchContent v0.15.3")

    set(HTTPLIB_REQUIRE_OPENSSL OFF CACHE BOOL "" FORCE)
    set(HTTPLIB_REQUIRE_ZLIB OFF CACHE BOOL "" FORCE)
    set(HTTPLIB_REQUIRE_BROTLI OFF CACHE BOOL "" FORCE)
    set(HTTPLIB_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        cpp_httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
        GIT_TAG 5c00bbf36ba8ff47b4fb97712fc38cb2884e5b98
        GIT_CONFIG ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
    )

    FetchContent_MakeAvailable(cpp_httplib)
else()
    nexus_print_dependency_mode(cpp-httplib "find_package")
    find_package(httplib REQUIRED CONFIG)
endif()
