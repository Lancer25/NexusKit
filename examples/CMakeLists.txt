add_subdirectory(core_status)

if(NEXUS_BUILD_FFMPEG)
    add_subdirectory(ffmpeg_probe)
endif()

if(TARGET nexus::screen)
    add_subdirectory(screen_capture)
endif()

if(TARGET nexus::usb)
    add_subdirectory(usb_hotplug)
endif()
