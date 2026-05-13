add_subdirectory(core_status)

if(NEXUS_BUILD_FFMPEG)
    add_subdirectory(ffmpeg_probe)
endif()
