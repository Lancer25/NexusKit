add_executable(nexus_example_media_probe
    main.cpp
)

target_link_libraries(nexus_example_media_probe
    PRIVATE
        nexus::media
)

if(WIN32 AND NEXUS_FFMPEG_RUNTIME_FILES)
    add_custom_command(
        TARGET nexus_example_media_probe
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${NEXUS_FFMPEG_RUNTIME_FILES}
            "$<TARGET_FILE_DIR:nexus_example_media_probe>"
        COMMAND_EXPAND_LISTS
    )
endif()
