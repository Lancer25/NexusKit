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
option(NEXUS_ENABLE_NET "Enable nexus_net" ON)
option(NEXUS_ENABLE_USB "Enable nexus_usb" OFF)
option(NEXUS_ENABLE_HID "Enable nexus_hid" OFF)
option(NEXUS_ENABLE_MEDIA "Enable nexus_media" OFF)
option(NEXUS_ENABLE_SCREEN "Enable nexus_screen" OFF)

if(NEXUS_ENABLE_NET AND NOT NEXUS_ENABLE_LOG)
    message(FATAL_ERROR "NEXUS_ENABLE_NET requires NEXUS_ENABLE_LOG for NexusKit diagnostic logging.")
endif()

if(NEXUS_ENABLE_HID AND NOT NEXUS_ENABLE_LOG)
    message(FATAL_ERROR "NEXUS_ENABLE_HID requires NEXUS_ENABLE_LOG for NexusKit diagnostic logging.")
endif()

if(NEXUS_ENABLE_USB AND NOT NEXUS_ENABLE_LOG)
    message(FATAL_ERROR "NEXUS_ENABLE_USB requires NEXUS_ENABLE_LOG for NexusKit diagnostic logging.")
endif()

if(NEXUS_ENABLE_USB AND NOT NEXUS_ENABLE_HID)
    message(FATAL_ERROR "NEXUS_ENABLE_USB requires NEXUS_ENABLE_HID for HID-backed device discovery.")
endif()

if(NEXUS_ENABLE_MEDIA AND NOT NEXUS_ENABLE_LOG)
    message(FATAL_ERROR "NEXUS_ENABLE_MEDIA requires NEXUS_ENABLE_LOG for NexusKit diagnostic logging.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/EHsc>)
endif()

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

if(NEXUS_BUILD_FFMPEG OR NEXUS_ENABLE_MEDIA)
    include(FFmpeg)
endif()

add_subdirectory(src/core)

if(NEXUS_ENABLE_LOG)
    add_subdirectory(src/log)
endif()

if(NEXUS_ENABLE_COMMON)
    add_subdirectory(src/common)
endif()

if(NEXUS_ENABLE_NET)
    add_subdirectory(src/net)
endif()

if(NEXUS_ENABLE_HID)
    add_subdirectory(src/hid)
endif()

if(NEXUS_ENABLE_USB)
    add_subdirectory(src/usb)
endif()

if(NEXUS_ENABLE_MEDIA)
    add_subdirectory(src/media)
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

install(CODE "
    file(GLOB _nexus_private_hidapi_pkgconfig
        \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig/hidapi*.pc\")
    if(_nexus_private_hidapi_pkgconfig)
        file(REMOVE \${_nexus_private_hidapi_pkgconfig})
    endif()
")
