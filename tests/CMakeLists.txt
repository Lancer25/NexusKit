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

    add_executable(nexus_net_tests
        net/http_tests.cpp
        net/tcp_tests.cpp
        net/udp_tests.cpp
    )

    target_link_libraries(nexus_net_tests
        PRIVATE
            nexus::net
            asio::asio
            httplib::httplib
            Catch2::Catch2WithMain
    )

    catch_discover_tests(nexus_net_tests)
endif()
