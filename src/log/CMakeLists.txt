include(Spdlog)

add_library(nexus_log ${NEXUS_LIBRARY_TYPE}
    logger.cpp
)

add_library(nexus::log ALIAS nexus_log)

target_include_directories(nexus_log
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(nexus_log PUBLIC cxx_std_17)

target_link_libraries(nexus_log
    PUBLIC
        nexus::core
    PRIVATE
        $<BUILD_INTERFACE:spdlog::spdlog_header_only>
)

if(NEXUS_BUILD_SHARED)
    target_compile_definitions(nexus_log
        PUBLIC NEXUS_LOG_SHARED
        PRIVATE NEXUS_LOG_BUILDING_LIBRARY
    )
endif()

if(MSVC)
    target_compile_options(nexus_log PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_log PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(nexus_log PROPERTIES
    OUTPUT_NAME nexus_log
    EXPORT_NAME log
)

install(
    TARGETS nexus_log
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
