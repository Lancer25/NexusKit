add_library(nexus_camera ${NEXUS_LIBRARY_TYPE}
    capturer.cpp
)

add_library(nexus::camera ALIAS nexus_camera)

target_include_directories(nexus_camera
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(nexus_camera PUBLIC cxx_std_17)

target_link_libraries(nexus_camera
    PUBLIC
        nexus::core
    PRIVATE
        nexus::common
        nexus::log
)

if(WIN32)
    # libuvc backend for Windows — requires externally provided uvc target
    if(TARGET uvc)
        target_link_libraries(nexus_camera PRIVATE uvc)
        if(TARGET LibUSB::LibUSB)
            target_link_libraries(nexus_camera PRIVATE LibUSB::LibUSB)
        endif()
        if(MSVC AND NEXUS_BUILD_LIBUVC)
            target_include_directories(nexus_camera PRIVATE
                "${CMAKE_BINARY_DIR}/_deps/libuvc-compat")
        endif()
        target_sources(nexus_camera PRIVATE uvc_capturer.cpp)
        target_compile_definitions(nexus_camera PRIVATE NEXUS_CAMERA_HAS_LIBUVC)
    endif()
elseif(UNIX)
    # V4L2 is a Linux kernel API — no link dependencies needed
    target_sources(nexus_camera PRIVATE v4l2_capturer.cpp)
    target_compile_definitions(nexus_camera PRIVATE NEXUS_CAMERA_HAS_V4L2)
endif()

if(NEXUS_BUILD_SHARED)
    target_compile_definitions(nexus_camera
        PUBLIC NEXUS_CAMERA_SHARED
        PRIVATE NEXUS_CAMERA_BUILDING_LIBRARY
    )
endif()

if(MSVC)
    target_compile_options(nexus_camera PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_camera PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(nexus_camera PROPERTIES
    OUTPUT_NAME nexus_camera
    EXPORT_NAME camera
)

install(
    TARGETS nexus_camera
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
