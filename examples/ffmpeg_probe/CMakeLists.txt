add_executable(nexus_example_ffmpeg_probe
    main.cpp
)

target_link_libraries(nexus_example_ffmpeg_probe
    PRIVATE
        FFmpeg::avcodec
        FFmpeg::avformat
        FFmpeg::avutil
)

if(WIN32 AND NEXUS_FFMPEG_RUNTIME_FILES)
    add_custom_command(
        TARGET nexus_example_ffmpeg_probe
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${NEXUS_FFMPEG_RUNTIME_FILES}
            "$<TARGET_FILE_DIR:nexus_example_ffmpeg_probe>"
        COMMAND_EXPAND_LISTS
    )
endif()
