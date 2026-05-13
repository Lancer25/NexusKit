include_guard(GLOBAL)

include(FetchContent)

if(NEXUS_BUILD_DEPS AND NEXUS_BUILD_HIDAPI)
    nexus_print_dependency_mode(hidapi "FetchContent 0.14.0")

    set(HIDAPI_BUILD_HIDTEST OFF CACHE BOOL "" FORCE)
    set(HIDAPI_WITH_TESTS OFF CACHE BOOL "" FORCE)
    set(HIDAPI_INSTALL_TARGETS OFF CACHE BOOL "" FORCE)

    FetchContent_Declare(
        hidapi
        GIT_REPOSITORY https://github.com/libusb/hidapi.git
        GIT_TAG 73d292a8f4d18f7fb532b9829b9a26ca8d864419
        GIT_CONFIG ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
    )

    FetchContent_MakeAvailable(hidapi)
else()
    nexus_print_dependency_mode(hidapi "find_package")
    find_package(hidapi REQUIRED CONFIG)
endif()
