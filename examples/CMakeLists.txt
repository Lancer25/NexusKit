add_subdirectory(core_status)

if(TARGET nexus::audio)
    add_subdirectory(audio_probe)
endif()

if(TARGET nexus::camera)
    add_subdirectory(camera_probe)
endif()

if(TARGET nexus::net)
    add_subdirectory(net_tcp_echo)
endif()

if(TARGET nexus::media)
    add_subdirectory(media_probe)
endif()

if(TARGET nexus::screen)
    add_subdirectory(screen_capture)
endif()

if(TARGET nexus::usb)
    add_subdirectory(usb_hotplug)
endif()
