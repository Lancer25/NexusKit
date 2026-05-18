include(Hidapi)

add_library(nexus_hid ${NEXUS_LIBRARY_TYPE}
    hid.cpp
)

add_library(nexus::hid ALIAS nexus_hid)

target_include_directories(nexus_hid
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(nexus_hid PUBLIC cxx_std_17)

target_link_libraries(nexus_hid
    PUBLIC
        nexus::core
    PRIVATE
        nexus::common
        nexus::log
        $<BUILD_INTERFACE:hidapi::hidapi>
)

if(NEXUS_BUILD_SHARED)
    target_compile_definitions(nexus_hid
        PUBLIC NEXUS_HID_SHARED
        PRIVATE NEXUS_HID_BUILDING_LIBRARY
    )
endif()

if(MSVC)
    target_compile_options(nexus_hid PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_hid PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(nexus_hid PROPERTIES
    OUTPUT_NAME nexus_hid
    EXPORT_NAME hid
)

install(
    TARGETS nexus_hid
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
