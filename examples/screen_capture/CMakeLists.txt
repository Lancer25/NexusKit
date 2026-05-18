add_executable(nexus_example_screen_capture
    main.cpp
)

target_link_libraries(nexus_example_screen_capture
    PRIVATE
        nexus::screen
)

if(MSVC)
    target_compile_options(nexus_example_screen_capture PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_example_screen_capture PRIVATE -Wall -Wextra -Wpedantic)
endif()
