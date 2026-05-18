add_executable(nexus_example_usb_hotplug
    main.cpp
)

target_link_libraries(nexus_example_usb_hotplug
    PRIVATE
        nexus::usb
)

if(MSVC)
    target_compile_options(nexus_example_usb_hotplug PRIVATE /W4 /permissive-)
else()
    target_compile_options(nexus_example_usb_hotplug PRIVATE -Wall -Wextra -Wpedantic)
endif()
