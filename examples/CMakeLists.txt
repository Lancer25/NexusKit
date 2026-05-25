add_subdirectory(core_status)

if(TARGET nexus::media)
    add_subdirectory(media_probe)
endif()

if(TARGET nexus::screen)
    add_subdirectory(screen_capture)
endif()

if(TARGET nexus::usb)
    add_subdirectory(usb_hotplug)
endif()
