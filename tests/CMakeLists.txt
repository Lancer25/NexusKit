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
    )

    target_link_libraries(nexus_common_tests
        PRIVATE
            nexus::common
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
        net/udp_tests.cpp
        net/websocket_tests.cpp
    )

    target_link_libraries(nexus_net_tests
        PRIVATE
            nexus::net
            nexus::log
            asio::asio
            httplib::httplib
            websocketpp::websocketpp
            Catch2::Catch2WithMain
    )

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

    catch_discover_tests(nexus_media_tests)
endif()
