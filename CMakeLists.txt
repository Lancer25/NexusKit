cmake_minimum_required(VERSION 3.24)

project(NexusKit
    VERSION 0.1.0
    DESCRIPTION "A cross-platform C++17 component library for systems, devices, networking, media, and screen capture."
    LANGUAGES CXX
)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/deps")

include(NexusGit)

option(NEXUS_BUILD_SHARED "Build NexusKit libraries as shared libraries" OFF)
option(NEXUS_BUILD_TESTS "Build NexusKit tests" ON)
option(NEXUS_BUILD_EXAMPLES "Build NexusKit examples" ON)
option(NEXUS_BUILD_DOCS "Build NexusKit documentation targets" OFF)

option(NEXUS_BUILD_DEPS "Build third-party dependencies from source" ON)
option(NEXUS_BUILD_FFMPEG "Build FFmpeg from source" OFF)
option(NEXUS_BUILD_OPENSSL "Build OpenSSL from source" OFF)
option(NEXUS_BUILD_HIDAPI "Build hidapi from source" OFF)
option(NEXUS_BUILD_PORTAUDIO "Build PortAudio from source" OFF)

option(NEXUS_ENABLE_LOG "Enable nexus_log" ON)
option(NEXUS_ENABLE_COMMON "Enable nexus_common" ON)
option(NEXUS_ENABLE_NET "Enable nexus_net" OFF)
option(NEXUS_ENABLE_USB "Enable nexus_usb" OFF)
option(NEXUS_ENABLE_HID "Enable nexus_hid" OFF)
option(NEXUS_ENABLE_MEDIA "Enable nexus_media" OFF)
option(NEXUS_ENABLE_SCREEN "Enable nexus_screen" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NEXUS_BUILD_SHARED)
    set(NEXUS_LIBRARY_TYPE SHARED)
else()
    set(NEXUS_LIBRARY_TYPE STATIC)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

if(NEXUS_BUILD_OPENSSL)
    include(OpenSSLSource)
endif()

if(NEXUS_BUILD_HIDAPI)
    include(Hidapi)
endif()

if(NEXUS_BUILD_PORTAUDIO)
    include(PortAudio)
endif()

if(NEXUS_BUILD_FFMPEG)
    include(FFmpeg)
endif()

add_subdirectory(src/core)

if(NEXUS_ENABLE_LOG)
    add_subdirectory(src/log)
endif()

if(NEXUS_ENABLE_COMMON)
    add_subdirectory(src/common)
endif()

if(NEXUS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(NEXUS_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/NexusKitConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/NexusKitConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/NexusKitConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NexusKit"
)

install(
    EXPORT NexusKitTargets
    NAMESPACE nexus::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NexusKit"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/NexusKitConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/NexusKitConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NexusKit"
)
