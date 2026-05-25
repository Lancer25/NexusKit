add_executable(nexus_example_audio_probe
    main.cpp
)

target_link_libraries(nexus_example_audio_probe
    PRIVATE
        nexus::audio
)
