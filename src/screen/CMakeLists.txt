add_library(nexus_screen ${NEXUS_LIBRARY_TYPE}
    screen.cpp
)

add_library(nexus::screen ALIAS nexus_screen)

target_include_directories(nexus_screen
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(nexus_screen PUBLIC cxx_std_17)

target_link_libraries(nexus_screen
    PUBLIC
        nexus::core
)

if(WIN32)
    target_sources(nexus_screen PRIVATE screen_win_dxgi.cpp)
    target_link_libraries(nexus_screen PRIVATE d3d11 dxgi)
    target_compile_definitions(nexus_screen PRIVATE NEXUS_SCREEN_WITH_DXGI)
endif()

if(NEXUS_BUILD_SHARED)
    target_compile_definitions(nexus_screen
        PUBLIC NEXUS_SCREEN_SHARED
        PRIVATE NEXUS_SCREEN_BUILDING_LIBRARY
    )
endif()

if(MSVC)
    target_compile_options(nexus_screen PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_screen PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(nexus_screen PROPERTIES
    OUTPUT_NAME nexus_screen
    EXPORT_NAME screen
)

install(
    TARGETS nexus_screen
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
