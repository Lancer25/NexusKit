add_executable(nexus_example_camera_probe
    main.cpp
)

target_link_libraries(nexus_example_camera_probe
    PRIVATE
        nexus::camera
)
