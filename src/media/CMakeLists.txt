add_library(nexus_media ${NEXUS_LIBRARY_TYPE}
    media.cpp
)

add_library(nexus::media ALIAS nexus_media)

target_include_directories(nexus_media
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_compile_features(nexus_media PUBLIC cxx_std_17)

target_link_libraries(nexus_media
    PUBLIC
        nexus::core
    PRIVATE
        nexus::log
)

if(TARGET FFmpeg::avutil
        AND TARGET FFmpeg::avcodec
        AND TARGET FFmpeg::avformat
        AND TARGET FFmpeg::swresample
        AND TARGET FFmpeg::swscale)
    target_compile_definitions(nexus_media PRIVATE NEXUS_MEDIA_WITH_FFMPEG)
    target_link_libraries(nexus_media
        PRIVATE
            $<BUILD_INTERFACE:FFmpeg::avutil>
            $<BUILD_INTERFACE:FFmpeg::avcodec>
            $<BUILD_INTERFACE:FFmpeg::avformat>
            $<BUILD_INTERFACE:FFmpeg::swresample>
            $<BUILD_INTERFACE:FFmpeg::swscale>
    )
endif()

if(NEXUS_BUILD_SHARED)
    target_compile_definitions(nexus_media
        PUBLIC NEXUS_MEDIA_SHARED
        PRIVATE NEXUS_MEDIA_BUILDING_LIBRARY
    )
endif()

if(MSVC)
    target_compile_options(nexus_media PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_media PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_target_properties(nexus_media PROPERTIES
    OUTPUT_NAME nexus_media
    EXPORT_NAME media
)

install(
    TARGETS nexus_media
    EXPORT NexusKitTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
