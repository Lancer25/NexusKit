include(Catch2)

add_executable(nexus_core_tests
    core/status_result_tests.cpp
)

target_link_libraries(nexus_core_tests
    PRIVATE
        nexus::core
        Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(nexus_core_tests)

if(TARGET nexus::log)
    add_executable(nexus_log_tests
        log/logger_tests.cpp
    )

    target_link_libraries(nexus_log_tests
        PRIVATE
            nexus::log
            Catch2::Catch2WithMain
    )

    catch_discover_tests(nexus_log_tests)
endif()

if(TARGET nexus::common)
    add_executable(nexus_common_tests
        common/json_tests.cpp
        common/xml_tests.cpp
        common/string_tests.cpp
        common/time_tests.cpp
        common/binary_tests.cpp
        common/platform_tests.cpp
        common/math_tests.cpp
        common/logging_tests.cpp
        common/status_tests.cpp
        common/thread_tests.cpp
    )

    target_link_libraries(nexus_common_tests
        PRIVATE
            nexus::common
            nexus::log
            Catch2::Catch2WithMain
    )

    catch_discover_tests(nexus_common_tests)
endif()

if(TARGET nexus::net)
    include(CppHttplib)
    include(Asio)
    include(Websocketpp)

    add_executable(nexus_net_tests
        net/http_tests.cpp
        net/tcp_tests.cpp
        net/tcp_listener_tests.cpp
        net/udp_tests.cpp
        net/websocket_tests.cpp
        net/websocket_server_tests.cpp
    )

    target_link_libraries(nexus_net_tests
        PRIVATE
            nexus::net
            nexus::common
            nexus::log
            asio::asio
            httplib::httplib
            websocketpp::websocketpp
            Catch2::Catch2WithMain
    )

    if(MSVC)
        target_link_options(nexus_net_tests PRIVATE
            "/DELAYLOAD:libssl-3-x64.dll"
            "/DELAYLOAD:libcrypto-3-x64.dll"
        )
    endif()

    catch_discover_tests(nexus_net_tests)
endif()

if(TARGET nexus::hid)
    add_executable(nexus_hid_tests
        hid/hid_tests.cpp
    )

    target_link_libraries(nexus_hid_tests
        PRIVATE
            nexus::hid
            Catch2::Catch2WithMain
    )

    catch_discover_tests(nexus_hid_tests)
endif()

if(TARGET nexus::usb)
    add_executable(nexus_usb_tests
        usb/usb_tests.cpp
    )

    target_link_libraries(nexus_usb_tests
        PRIVATE
            nexus::usb
            Catch2::Catch2WithMain
    )

    target_include_directories(nexus_usb_tests
        PRIVATE
            "${PROJECT_SOURCE_DIR}/src/usb"
    )

    catch_discover_tests(nexus_usb_tests)
endif()

if(TARGET nexus::media)
    add_executable(nexus_media_tests
        media/media_tests.cpp
    )

    target_link_libraries(nexus_media_tests
        PRIVATE
            nexus::media
            nexus::log
            Catch2::Catch2WithMain
    )

    if(WIN32 AND NEXUS_FFMPEG_RUNTIME_FILES)
        add_custom_command(
            TARGET nexus_media_tests
            POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                ${NEXUS_FFMPEG_RUNTIME_FILES}
                "$<TARGET_FILE_DIR:nexus_media_tests>"
            COMMAND_EXPAND_LISTS
        )
    endif()

    catch_discover_tests(nexus_media_tests
        DL_PATHS "$<TARGET_FILE_DIR:nexus_media_tests>"
    )
endif()

if(TARGET nexus::screen)
    add_executable(nexus_screen_tests
        screen/screen_tests.cpp
    )

    target_link_libraries(nexus_screen_tests
        PRIVATE
            nexus::screen
            Catch2::Catch2WithMain
    )

    target_include_directories(nexus_screen_tests
        PRIVATE
            "${PROJECT_SOURCE_DIR}/src/screen"
    )

    catch_discover_tests(nexus_screen_tests)
endif()

if(TARGET nexus::audio)
    add_executable(nexus_audio_tests
        audio/audio_tests.cpp
        audio/audio_player_tests.cpp
    )

    target_link_libraries(nexus_audio_tests
        PRIVATE
            nexus::audio
            Catch2::Catch2WithMain
    )

    catch_discover_tests(nexus_audio_tests)
endif()

if(TARGET nexus::camera)
    add_executable(nexus_camera_tests
        camera/camera_tests.cpp
    )

    target_link_libraries(nexus_camera_tests
        PRIVATE
            nexus::camera
            Catch2::Catch2WithMain
    )

    catch_discover_tests(nexus_camera_tests)
endif()
