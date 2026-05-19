include_guard(GLOBAL)

include(FetchContent)

if(NOT NEXUS_BUILD_DEPS)
    nexus_print_dependency_mode(libusb "find_package")
    find_package(LibUSB QUIET)
    if(NOT TARGET LibUSB::LibUSB AND LibUSB_FOUND)
        add_library(LibUSB::LibUSB ALIAS libusb-1.0)
    endif()
    return()
endif()

nexus_print_dependency_mode(libusb "FetchContent v1.0.27 (custom CMake)")

FetchContent_Declare(
    libusb_src
    GIT_REPOSITORY https://github.com/libusb/libusb.git
    GIT_TAG        v1.0.27
    GIT_CONFIG     ${NEXUS_GIT_EFFECTIVE_CONFIG_ARGS}
)

FetchContent_MakeAvailable(libusb_src)

# libusb Windows static library — compiled from the same sources as the MSVC project
set(LIBUSB_SRC_DIR "${libusb_src_SOURCE_DIR}/libusb")

add_library(libusb-1.0 STATIC
    "${LIBUSB_SRC_DIR}/core.c"
    "${LIBUSB_SRC_DIR}/descriptor.c"
    "${LIBUSB_SRC_DIR}/os/events_windows.c"
    "${LIBUSB_SRC_DIR}/hotplug.c"
    "${LIBUSB_SRC_DIR}/io.c"
    "${LIBUSB_SRC_DIR}/strerror.c"
    "${LIBUSB_SRC_DIR}/sync.c"
    "${LIBUSB_SRC_DIR}/os/threads_windows.c"
    "${LIBUSB_SRC_DIR}/os/windows_common.c"
    "${LIBUSB_SRC_DIR}/os/windows_usbdk.c"
    "${LIBUSB_SRC_DIR}/os/windows_winusb.c"
)

target_include_directories(libusb-1.0
    PUBLIC
        $<BUILD_INTERFACE:${LIBUSB_SRC_DIR}>
    PRIVATE
        "${libusb_src_SOURCE_DIR}/msvc"
        "${LIBUSB_SRC_DIR}/os"
)

target_compile_definitions(libusb-1.0
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        UNICODE
        _UNICODE
)

target_link_libraries(libusb-1.0
    PUBLIC
        setupapi
        winusb
        ole32
)

if(MSVC)
    target_compile_options(libusb-1.0 PRIVATE /W3)
endif()

add_library(LibUSB::LibUSB ALIAS libusb-1.0)

install(TARGETS libusb-1.0
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
