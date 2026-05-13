include(NlohmannJson)

add_library(nexus_common ${NEXUS_LIBRARY_TYPE}
    json.cpp
)

add_library(nexus::common ALIAS nexus_common)

target_include_directories(nexus_common
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(nexus_common PUBLIC cxx_std_17)

target_link_libraries(nexus_common
    PUBLIC
        nexus::core
    PRIVATE
        $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>
)

if(NEXUS_BUILD_SHARED)
    target_compile_definitions(nexus_common
        PUBLIC NEXUS_COMMON_SHARED
        PRIVATE NEXUS_COMMON_BUILDING_LIBRARY
    )
endif()

if(MSVC)
    target_compile_options(nexus_common PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_common PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(nexus_common PROPERTIES
    OUTPUT_NAME nexus_common
    EXPORT_NAME common
)

install(
    TARGETS nexus_common
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
