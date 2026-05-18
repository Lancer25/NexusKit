add_library(nexus_audio ${NEXUS_LIBRARY_TYPE}
    capturer.cpp
)

add_library(nexus::audio ALIAS nexus_audio)

target_include_directories(nexus_audio
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(nexus_audio PUBLIC cxx_std_17)

target_link_libraries(nexus_audio
    PUBLIC
        nexus::core
    PRIVATE
        nexus::common
        nexus::log
)

if(WIN32)
    target_link_libraries(nexus_audio PRIVATE ole32.lib)
    target_sources(nexus_audio PRIVATE wasapi_capturer.cpp)
    target_compile_definitions(nexus_audio PRIVATE NEXUS_AUDIO_HAS_WASAPI)
elseif(UNIX)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(PULSEAUDIO IMPORTED_TARGET libpulse-simple)
    endif()
    if(TARGET PkgConfig::PULSEAUDIO)
        target_link_libraries(nexus_audio PRIVATE PkgConfig::PULSEAUDIO)
        target_sources(nexus_audio PRIVATE pulse_capturer.cpp)
        target_compile_definitions(nexus_audio PRIVATE NEXUS_AUDIO_HAS_PULSE)
    endif()
endif()

if(NEXUS_BUILD_SHARED)
    target_compile_definitions(nexus_audio
        PUBLIC NEXUS_AUDIO_SHARED
        PRIVATE NEXUS_AUDIO_BUILDING_LIBRARY
    )
endif()

if(MSVC)
    target_compile_options(nexus_audio PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_audio PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(nexus_audio PROPERTIES
    OUTPUT_NAME nexus_audio
    EXPORT_NAME audio
)

install(
    TARGETS nexus_audio
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
