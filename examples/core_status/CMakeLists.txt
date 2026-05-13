add_executable(nexus_example_core_status
    main.cpp
)

target_link_libraries(nexus_example_core_status
    PRIVATE
        nexus::core
)
