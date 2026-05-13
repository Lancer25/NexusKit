add_library(nexus_core ${NEXUS_LIBRARY_TYPE}
    version.cpp
)

add_library(nexus::core ALIAS nexus_core)

target_include_directories(nexus_core
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(nexus_core PUBLIC cxx_std_17)

if(NEXUS_BUILD_SHARED)
    target_compile_definitions(nexus_core
        PUBLIC NEXUS_CORE_SHARED
        PRIVATE NEXUS_CORE_BUILDING_LIBRARY
    )
endif()

if(MSVC)
    target_compile_options(nexus_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(nexus_core PROPERTIES
    OUTPUT_NAME nexus_core
    EXPORT_NAME core
)

install(
    TARGETS nexus_core
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install(
    DIRECTORY "${PROJECT_SOURCE_DIR}/include/nexus"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)
