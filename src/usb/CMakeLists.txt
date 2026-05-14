add_library(nexus_usb ${NEXUS_LIBRARY_TYPE}
    usb.cpp
)

add_library(nexus::usb ALIAS nexus_usb)

target_include_directories(nexus_usb
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(nexus_usb PUBLIC cxx_std_17)

target_link_libraries(nexus_usb
    PUBLIC
        nexus::core
    PRIVATE
        nexus::hid
        nexus::log
)

if(NEXUS_BUILD_SHARED)
    target_compile_definitions(nexus_usb
        PUBLIC NEXUS_USB_SHARED
        PRIVATE NEXUS_USB_BUILDING_LIBRARY
    )
endif()

if(MSVC)
    target_compile_options(nexus_usb PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_usb PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(nexus_usb PROPERTIES
    OUTPUT_NAME nexus_usb
    EXPORT_NAME usb
)

install(
    TARGETS nexus_usb
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
